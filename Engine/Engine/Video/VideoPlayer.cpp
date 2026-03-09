#include "VideoPlayer.h"
#include "../Audio/AudioSystem.h"
#include "../Resources.h"
#include "../Path.h"
#include "../Logger.h"
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
      // Retrieve ring buffer pointer stored as userdata on the sound
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

      // Zero-fill remainder if ring didn't have enough
      if (num_read < float_count)
        std::memset(out + num_read, 0, (float_count - num_read) * sizeof(float));

      return FMOD_OK;
    }

    // ----------------------------------------------------------------
    // VideoPlayer::Impl — hidden implementation
    // ----------------------------------------------------------------
    struct VideoPlayer::Impl
    {
      plm_t* plm = nullptr;

      // Video frame texture
      Graphics::Texture texture;
      std::vector<uint8_t> rgba_buffer;
      bool frame_dirty = false;

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
        if (texture && texture->id != 0)
        {
          glDeleteTextures(1, &texture->id);
          texture->id = 0;
        }
        texture.reset();
        rgba_buffer.clear();
        ring.clear();
        playing = false;
        paused = false;
        frame_dirty = false;
        width = 0;
        height = 0;
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

        // Ring buffer: 1 second of stereo float audio
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

      // pl_mpeg video callback — called from plm_decode() on main thread
      static void on_video(plm_t* /*mpeg*/, plm_frame_t* frame, void* user)
      {
        auto* impl = static_cast<Impl*>(user);
        // Convert YCrCb to RGBA, writing into our pre-allocated buffer.
        // plm_frame_to_rgba leaves alpha untouched; buffer was init'd to 255.
        plm_frame_to_rgba(frame, impl->rgba_buffer.data(), impl->width * 4);
        impl->frame_dirty = true;
      }

      // pl_mpeg audio callback — called from plm_decode() on main thread
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

      // Probe for stream headers if they weren't found immediately
      if (!plm_has_headers(m_impl->plm))
      {
        plm_probe(m_impl->plm, 128 * 1024);
      }

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

      // Allocate RGBA buffer (alpha channel pre-filled to 255)
      m_impl->rgba_buffer.resize(
        static_cast<size_t>(m_impl->width) * m_impl->height * 4, 255);

      // Create OpenGL texture for video frames
      auto tex = std::make_shared<Graphics::TextureData>();
      tex->width = m_impl->width;
      tex->height = m_impl->height;
      tex->filter = Graphics::TextureFilter::Linear;
      tex->path = full_path;

      glGenTextures(1, &tex->id);
      glBindTexture(GL_TEXTURE_2D, tex->id);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
        m_impl->width, m_impl->height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

      m_impl->texture = tex;

      // Install pl_mpeg callbacks for both video and audio
      plm_set_video_decode_callback(m_impl->plm, Impl::on_video, m_impl.get());
      plm_set_audio_decode_callback(m_impl->plm, Impl::on_audio, m_impl.get());

      // Audio lead time: how far ahead audio is decoded relative to video
      plm_set_audio_lead_time(m_impl->plm, 1152.0 * 4.0 / 44100.0);

      // Initialize FMOD audio
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

    void VideoPlayer::update(float dt)
    {
      if (!m_impl->plm || !m_impl->playing || m_impl->paused)
        return;

      // Keep FMOD channel volume in sync with master volume changes
      m_impl->apply_volume();

      // Advance decoder — this fires on_video / on_audio callbacks as needed
      plm_decode(m_impl->plm, static_cast<double>(dt));

      // Upload the latest decoded frame to GPU
      if (m_impl->frame_dirty && m_impl->texture)
      {
        glBindTexture(GL_TEXTURE_2D, m_impl->texture->id);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
          m_impl->width, m_impl->height,
          GL_RGBA, GL_UNSIGNED_BYTE,
          m_impl->rgba_buffer.data());
        m_impl->frame_dirty = false;
      }

      if (plm_has_ended(m_impl->plm))
      {
        m_impl->playing = false;
        m_impl->stop_audio();
      }
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

    Graphics::Texture VideoPlayer::get_texture() const
    {
      return m_impl->texture;
    }

  } // namespace Video
} // namespace Engine
