//-----------------------------------------------------------------------------------
// file:    AudioPlayerInternal.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Internal data structures and utilities for the audio system.
//          Defines SoundData structure for FMOD sound resources and provides
//          access to the global FMOD system instance. Handles low-level audio
//          resource management and FMOD integration details.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Precompiled.h"
#include "../TypeDef.h"
#include "../Logger.h"
#include <string>
#include <memory>

// Forward declare FMOD types
namespace FMOD {
  class System;
  class Sound;
  class Channel;
}

namespace Engine {
  namespace Audio {

    // Get the global FMOD system instance
    FMOD::System* get_fmod_system();

    // Sound resource data structure
    struct SoundData {
      FMOD::Sound* sound = nullptr;
      bool is_stream = false; // true for music (streamed), false for sound effects (loaded in memory)

      SoundData(const std::string& path, bool stream);
      ~SoundData();

      // Delete copy constructor and assignment operator
      SoundData(const SoundData&) = delete;
      SoundData& operator=(const SoundData&) = delete;
      SoundData(SoundData&&) = delete;

      // Default constructor
      SoundData();
    };

  } // namespace Audio
} // namespace Engine