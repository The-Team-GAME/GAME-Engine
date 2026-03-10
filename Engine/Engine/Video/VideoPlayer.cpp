#include "VideoPlayer.h"
#include "../Audio/AudioSystem.h"
#include "../Resources.h"
#include "../Path.h"
#include "../Logger.h"
#include "../Time.h"
#include "../Graphics/Shader.h"
#include <GL/glew.h>
#include <fmod.hpp>
#include <mutex>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#define PL_MPEG_IMPLEMENTATION
#include <pl_mpeg.h>

namespace Engine {
  namespace Video {

    // ----------------------------------------------------------------
    // Ring buffer for bridging pl_mpeg audio decode (main thread) to
    // FMOD PCM read callback (mixer thread).
    // ----------------------------------------------------------------
    class AudioRingBuffer
    {
    public:
      void init(size_t capacity_samples)
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_buffer.resize(capacity_samples, 0.0f);
        m_read = 0;
        m_write = 0;
        m_count = 0;
      }

      size_t write(const float* data, size_t count)
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t cap = m_buffer.size();
        size_t space = cap - m_count;
        size_t to_write = (count < space) ? count : space;
        for (size_t i = 0; i < to_write; ++i)
        {
          m_buffer[m_write] = data[i];
          m_write = (m_write + 1) % cap;
        }
        m_count += to_write;
        return to_write;
      }

      size_t read(float* dest, size_t count)
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t to_read = (count < m_count) ? count : m_count;
        for (size_t i = 0; i < to_read; ++i)
        {
          dest[i] = m_buffer[m_read];
          m_read = (m_read + 1) % m_buffer.size();
        }
        m_count -= to_read;
        return to_read;
      }

      void clear()
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_read = 0;
        m_write = 0;
        m_count = 0;
      }

    private:
      std::vector<float> m_buffer;
      size_t m_read = 0;
      size_t m_write = 0;
      size_t m_count = 0;
      mutable std::mutex m_mutex;
    };

    // ----------------------------------------------------------------
    // FMOD PCM read callback — runs on FMOD's mixer thread
    // ----------------------------------------------------------------
    static FMOD_RESULT F_CALL fmod_pcm_read(
      FMOD_SOUND* sound, void* data, unsigned int datalen)
    {
      FMOD::Sound* snd = reinterpret_cast<FMOD::Sound*>(sound);
      void* ud = nullptr;
      snd->getUserData(&ud);
      auto* ring = static_cast<AudioRingBuffer*>(ud);

      if (!ring)
      {
        std::memset(data, 0, datalen);
        return FMOD_OK;
      }

      size_t float_count = datalen / sizeof(float);
      float* out = static_cast<float*>(data);
      size_t num_read = ring->read(out, float_count);

      if (num_read < float_count)
        std::memset(out + num_read, 0, (float_count - num_read) * sizeof(float));

      return FMOD_OK;
    }

    // ----------------------------------------------------------------
    // Helper: create a single-channel GL_R8 texture
    // ----------------------------------------------------------------
    static GLuint create_plane_texture(int w, int h)
    {
      GLuint tex = 0;
      glGenTextures(1, &tex);
      glBindTexture(GL_TEXTURE_2D, tex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
      return tex;
    }

    // ----------------------------------------------------------------
    // VideoPlayer::Impl — hidden implementation
    // ----------------------------------------------------------------
    struct VideoPlayer::Impl
    {
      plm_t* plm = nullptr;

      // YCbCr plane textures (uploaded to GPU each frame)
      GLuint tex_y = 0;
      GLuint tex_cb = 0;
      GLuint tex_cr = 0;

      // CPU-side plane buffers (written by on_video callback)
      std::vector<uint8_t> buf_y;
      std::vector<uint8_t> buf_cb;
      std::vector<uint8_t> buf_cr;
      bool frame_dirty = false;

      // Plane dimensions (may differ from display width/height due to macroblock rounding)
      int plane_y_w = 0, plane_y_h = 0;
      int plane_c_w = 0, plane_c_h = 0;

      // Rendering resources
      GLuint quad_vao = 0;
      GLuint quad_vbo = 0;
      std::unique_ptr<Graphics::Shader> shader;

      // FMOD audio
      FMOD::System* core_system = nullptr;
      FMOD::Sound* fmod_sound = nullptr;
      FMOD::Channel* fmod_channel = nullptr;
      AudioRingBuffer ring;

      // State
      bool playing = false;
      bool paused = false;
      float volume = 1.0f;
      int width = 0;
      int height = 0;

      void init_quad()
      {
        // Unit quad: position (-1..1), texcoord (0..1)
        float verts[] = {
          -1.0f, -1.0f,  0.0f, 1.0f,   // bottom-left  (tex V flipped: 1)
           1.0f, -1.0f,  1.0f, 1.0f,   // bottom-right
           1.0f,  1.0f,  1.0f, 0.0f,   // top-right
          -1.0f, -1.0f,  0.0f, 1.0f,   // bottom-left
           1.0f,  1.0f,  1.0f, 0.0f,   // top-right
          -1.0f,  1.0f,  0.0f, 0.0f,   // top-left
        };

        glGenVertexArrays(1, &quad_vao);
        glGenBuffers(1, &quad_vbo);
        glBindVertexArray(quad_vao);
        glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        // location 0: position (vec2)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        // location 1: texcoord (vec2)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);
      }

      void cleanup()
      {
        if (fmod_channel)
        {
          fmod_channel->stop();
          fmod_channel = nullptr;
        }
        if (fmod_sound)
        {
          fmod_sound->release();
          fmod_sound = nullptr;
        }
        if (plm)
        {
          plm_destroy(plm);
          plm = nullptr;
        }
        auto delete_tex = [](GLuint& t) { if (t) { glDeleteTextures(1, &t); t = 0; } };
        delete_tex(tex_y);
        delete_tex(tex_cb);
        delete_tex(tex_cr);

        if (quad_vbo) { glDeleteBuffers(1, &quad_vbo); quad_vbo = 0; }
        if (quad_vao) { glDeleteVertexArrays(1, &quad_vao); quad_vao = 0; }
        shader.reset();

        buf_y.clear();
        buf_cb.clear();
        buf_cr.clear();
        ring.clear();
        playing = false;
        paused = false;
        frame_dirty = false;
        width = 0;
        height = 0;
        plane_y_w = plane_y_h = plane_c_w = plane_c_h = 0;
      }

      bool init_audio()
      {
        core_system = static_cast<FMOD::System*>(AudioSystem::getCoreSystem());
        if (!core_system)
        {
          Log::warning("[VideoPlayer] FMOD core system unavailable, video will play without audio");
          return false;
        }

        int samplerate = plm_get_samplerate(plm);
        int num_audio = plm_get_num_audio_streams(plm);
        if (num_audio == 0 || samplerate == 0)
        {
          Log::info("[VideoPlayer] No audio stream in video file");
          return false;
        }

        size_t ring_size = static_cast<size_t>(samplerate) * 2;
        ring.init(ring_size);

        FMOD_CREATESOUNDEXINFO exinfo = {};
        exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
        exinfo.numchannels = 2;
        exinfo.defaultfrequency = samplerate;
        exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
        exinfo.decodebuffersize = 1152 * 4;
        exinfo.length = static_cast<unsigned int>(
          exinfo.defaultfrequency * exinfo.numchannels * sizeof(float) * 10);
        exinfo.pcmreadcallback = fmod_pcm_read;

        FMOD_RESULT result = core_system->createSound(
          nullptr,
          FMOD_OPENUSER | FMOD_CREATESTREAM | FMOD_LOOP_NORMAL | FMOD_2D,
          &exinfo,
          &fmod_sound);

        if (result != FMOD_OK)
        {
          Log::error("[VideoPlayer] Failed to create FMOD sound: {}", static_cast<int>(result));
          return false;
        }

        fmod_sound->setUserData(&ring);
        return true;
      }

      void start_audio()
      {
        if (!fmod_sound || !core_system) return;

        FMOD_RESULT result = core_system->playSound(fmod_sound, nullptr, true, &fmod_channel);
        if (result != FMOD_OK)
        {
          Log::error("[VideoPlayer] Failed to play FMOD sound: {}", static_cast<int>(result));
          fmod_channel = nullptr;
          return;
        }
        apply_volume();
        fmod_channel->setPaused(false);
      }

      void apply_volume()
      {
        if (!fmod_channel) return;
        float master_vol = AudioSystem::getMasterVolume();
        fmod_channel->setVolume(volume * master_vol);
      }

      void stop_audio()
      {
        if (fmod_channel)
        {
          fmod_channel->stop();
          fmod_channel = nullptr;
        }
        ring.clear();
      }

      // pl_mpeg video callback — copies raw YCbCr planes
      static void on_video(plm_t* /*mpeg*/, plm_frame_t* frame, void* user)
      {
        auto* impl = static_cast<Impl*>(user);

        // Copy each plane row-by-row (plane stride may differ from display width)
        auto copy_plane = [](const plm_plane_t& plane, std::vector<uint8_t>& buf, int w, int h) {
          for (int row = 0; row < h; ++row)
            std::memcpy(buf.data() + row * w, plane.data + row * plane.width, w);
          };

        copy_plane(frame->y, impl->buf_y, impl->plane_y_w, impl->plane_y_h);
        copy_plane(frame->cb, impl->buf_cb, impl->plane_c_w, impl->plane_c_h);
        copy_plane(frame->cr, impl->buf_cr, impl->plane_c_w, impl->plane_c_h);

        impl->frame_dirty = true;
      }

      // pl_mpeg audio callback
      static void on_audio(plm_t* /*mpeg*/, plm_samples_t* samples, void* user)
      {
        auto* impl = static_cast<Impl*>(user);
        impl->ring.write(samples->interleaved, samples->count * 2);
      }
    };

    // ----------------------------------------------------------------
    // VideoPlayer public implementation
    // ----------------------------------------------------------------

    VideoPlayer::VideoPlayer()
      : m_impl(std::make_unique<Impl>())
    {
    }

    VideoPlayer::~VideoPlayer()
    {
      close();
    }

    bool VideoPlayer::open(const std::string& path, bool append_resource_path)
    {
      close();

      std::string full_path = append_resource_path
        ? Engine::Path(Resources::resource_path).join(path)
        : Engine::Path(path).normalize();

      m_impl->plm = plm_create_with_filename(full_path.c_str());
      if (!m_impl->plm)
      {
        Log::error("[VideoPlayer] Failed to open video file: {}", full_path);
        return false;
      }

      if (!plm_has_headers(m_impl->plm))
        plm_probe(m_impl->plm, 128 * 1024);

      m_impl->width = plm_get_width(m_impl->plm);
      m_impl->height = plm_get_height(m_impl->plm);
      if (m_impl->width == 0 || m_impl->height == 0)
      {
        Log::error("[VideoPlayer] Invalid video dimensions in: {}", full_path);
        Log::error("[VideoPlayer] File must be an MPEG-PS (.mpg) container. "
          "Convert with: ffmpeg -i input.mp4 -c:v mpeg1video -c:a mp2 -f mpeg output.mpg");
        close();
        return false;
      }

      // Plane dimensions: Y is full size, Cb/Cr are half (4:2:0)
      // pl_mpeg rounds up to macroblock boundaries (16px) internally,
      // but we only upload the display-sized portion.
      m_impl->plane_y_w = m_impl->width;
      m_impl->plane_y_h = m_impl->height;
      m_impl->plane_c_w = m_impl->width / 2;
      m_impl->plane_c_h = m_impl->height / 2;

      m_impl->buf_y.resize(static_cast<size_t>(m_impl->plane_y_w) * m_impl->plane_y_h, 0);
      m_impl->buf_cb.resize(static_cast<size_t>(m_impl->plane_c_w) * m_impl->plane_c_h, 128);
      m_impl->buf_cr.resize(static_cast<size_t>(m_impl->plane_c_w) * m_impl->plane_c_h, 128);

      // Create GL_R8 textures for the 3 planes
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      m_impl->tex_y = create_plane_texture(m_impl->plane_y_w, m_impl->plane_y_h);
      m_impl->tex_cb = create_plane_texture(m_impl->plane_c_w, m_impl->plane_c_h);
      m_impl->tex_cr = create_plane_texture(m_impl->plane_c_w, m_impl->plane_c_h);

      // Load YCbCr→RGB shader (once, reused across open/close cycles)
      if (!m_impl->shader)
      {
        m_impl->shader = Graphics::Shader::load_from_file(
          "Assets/shaders/video_yuv.vert", "Assets/shaders/video_yuv.frag");
        if (!m_impl->shader || !m_impl->shader->is_valid())
        {
          Log::error("[VideoPlayer] Failed to load YCbCr shader");
          close();
          return false;
        }
      }

      // Create quad VAO/VBO if not yet created
      if (!m_impl->quad_vao)
        m_impl->init_quad();

      // Install pl_mpeg callbacks
      plm_set_video_decode_callback(m_impl->plm, Impl::on_video, m_impl.get());
      plm_set_audio_decode_callback(m_impl->plm, Impl::on_audio, m_impl.get());
      plm_set_audio_lead_time(m_impl->plm, 1152.0 * 4.0 / 44100.0);

      m_impl->init_audio();

      Log::info("[VideoPlayer] Opened: {} ({}x{}, {:.1f}s)",
        full_path, m_impl->width, m_impl->height, plm_get_duration(m_impl->plm));
      return true;
    }

    void VideoPlayer::close()
    {
      m_impl->cleanup();
    }

    void VideoPlayer::play()
    {
      if (!m_impl->plm) return;
      m_impl->playing = true;
      m_impl->paused = false;
      plm_rewind(m_impl->plm);
      m_impl->ring.clear();
      m_impl->start_audio();
    }

    void VideoPlayer::stop()
    {
      if (!m_impl->plm) return;
      m_impl->playing = false;
      m_impl->paused = false;
      m_impl->stop_audio();
    }

    void VideoPlayer::pause(bool paused)
    {
      if (!m_impl->plm) return;
      m_impl->paused = paused;
      if (m_impl->fmod_channel)
        m_impl->fmod_channel->setPaused(paused);
    }

    void VideoPlayer::set_looping(bool loop)
    {
      if (!m_impl->plm) return;
      plm_set_loop(m_impl->plm, loop ? 1 : 0);
    }

    void VideoPlayer::set_volume(float v)
    {
      m_impl->volume = v;
      m_impl->apply_volume();
    }

    void VideoPlayer::render(float vres_w, float vres_h)
    {
      if (!m_impl->shader || !m_impl->quad_vao || m_impl->width == 0)
        return;

      // Advance decoder using real frame delta (runs once per render frame)
      if (m_impl->plm && m_impl->playing && !m_impl->paused)
      {
        m_impl->apply_volume();
        plm_decode(m_impl->plm, static_cast<double>(Time::get_real_dt()));

        if (plm_has_ended(m_impl->plm))
        {
          m_impl->playing = false;
          m_impl->stop_audio();
        }
      }

      // Upload YCbCr planes to GPU
      if (m_impl->frame_dirty)
      {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBindTexture(GL_TEXTURE_2D, m_impl->tex_y);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
          m_impl->plane_y_w, m_impl->plane_y_h,
          GL_RED, GL_UNSIGNED_BYTE, m_impl->buf_y.data());

        glBindTexture(GL_TEXTURE_2D, m_impl->tex_cb);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
          m_impl->plane_c_w, m_impl->plane_c_h,
          GL_RED, GL_UNSIGNED_BYTE, m_impl->buf_cb.data());

        glBindTexture(GL_TEXTURE_2D, m_impl->tex_cr);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
          m_impl->plane_c_w, m_impl->plane_c_h,
          GL_RED, GL_UNSIGNED_BYTE, m_impl->buf_cr.data());

        m_impl->frame_dirty = false;
      }

      // Compute aspect-ratio-preserving size in NDC
      float vid_w = static_cast<float>(m_impl->width);
      float vid_h = static_cast<float>(m_impl->height);
      float scale = (std::min)(vres_w / vid_w, vres_h / vid_h);
      float ndc_half_w = (vid_w * scale) / vres_w;  // in 0..1 range, maps to -1..1 via shader
      float ndc_half_h = (vid_h * scale) / vres_h;

      m_impl->shader->use();

      // Centered at origin (NDC 0,0)
      m_impl->shader->set_vec2("u_pos", 0.0f, 0.0f);
      m_impl->shader->set_vec2("u_size", ndc_half_w, ndc_half_h);

      // Bind the 3 YCbCr plane textures
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, m_impl->tex_y);
      m_impl->shader->set_int("u_tex_y", 0);

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, m_impl->tex_cb);
      m_impl->shader->set_int("u_tex_cb", 1);

      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, m_impl->tex_cr);
      m_impl->shader->set_int("u_tex_cr", 2);

      // Draw the quad
      glBindVertexArray(m_impl->quad_vao);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      glBindVertexArray(0);

      // Reset active texture to 0
      glActiveTexture(GL_TEXTURE0);
    }

    bool VideoPlayer::is_playing() const
    {
      return m_impl->playing && !m_impl->paused;
    }

    bool VideoPlayer::is_paused() const
    {
      return m_impl->paused;
    }

    bool VideoPlayer::has_ended() const
    {
      if (!m_impl->plm) return true;
      return plm_has_ended(m_impl->plm) != 0;
    }

    float VideoPlayer::get_volume() const
    {
      return m_impl->volume;
    }

    int VideoPlayer::get_width() const
    {
      return m_impl->width;
    }

    int VideoPlayer::get_height() const
    {
      return m_impl->height;
    }

    double VideoPlayer::get_duration() const
    {
      if (!m_impl->plm) return 0.0;
      return plm_get_duration(m_impl->plm);
    }

    double VideoPlayer::get_time() const
    {
      if (!m_impl->plm) return 0.0;
      return plm_get_time(m_impl->plm);
    }

  } // namespace Video
} // namespace Engine
