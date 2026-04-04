//-----------------------------------------------------------------------------------
// file:    VideoPlayer.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Public interface for MPEG video playback with integrated audio and rendering controls.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
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

      // Open an .mpg file for playback (path relative to Assets/ by default)
      bool open(const std::string& path, bool append_resource_path = true);

      // Close and release all resources
      void close();

      // Playback controls
      void play();
      void stop();
      void pause(bool paused);
      void set_looping(bool loop);
      void set_volume(float volume);

      // Render the current video frame as a centered, aspect-ratio-preserved
      // quad within the given virtual resolution.
      // Automatically advances the decoder each frame.
      void render(float vres_w, float vres_h);

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

    private:
#pragma warning(push)
#pragma warning(disable: 4251)
      struct Impl;
      std::unique_ptr<Impl> m_impl;
#pragma warning(pop)
    };

  } // namespace Video
} // namespace Engine


