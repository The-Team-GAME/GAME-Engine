#pragma once

#include "../EngineExport.h"
#include "../Graphics/RendererInternal.h"
#include <memory>
#include <string>

namespace Engine {
  namespace Video {

    class ENGINE_API VideoPlayer
    {
    public:
      VideoPlayer();
      ~VideoPlayer();

      // Non-copyable
      VideoPlayer(const VideoPlayer&) = delete;
      VideoPlayer& operator=(const VideoPlayer&) = delete;

      // Open an .mpg file for playback
      bool open(const std::string& path);

      // Close and release all resources
      void close();

      // Playback controls
      void play();
      void stop();
      void pause(bool paused);
      void set_looping(bool loop);
      void set_volume(float volume);

      // Must be called every frame while playing
      void update(float dt);

      // Query state
      bool is_playing() const;
      bool is_paused() const;
      bool has_ended() const;
      float get_volume() const;

      // Video info
      int get_width() const;
      int get_height() const;
      double get_duration() const;
      double get_time() const;

      // Returns the texture containing the current video frame.
      // Use with Graphics::draw_texture() or draw_texture_ui().
      Graphics::Texture get_texture() const;

    private:
#pragma warning(push)
#pragma warning(disable: 4251)
      struct Impl;
      std::unique_ptr<Impl> m_impl;
#pragma warning(pop)
    };

  } // namespace Video
} // namespace Engine
