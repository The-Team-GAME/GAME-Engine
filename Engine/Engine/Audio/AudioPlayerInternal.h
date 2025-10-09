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