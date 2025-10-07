#pragma once

#include "../Precompiled.h"
#include <memory>
#include <string>
#include "../Logger.h"
#include "../TypeDef.h"
#include <fmod.hpp>

namespace Engine {
  namespace Audio {

    // Get the global FMOD system instance
    FMOD::System* get_fmod_system();

    // Audio data structure
    struct SoundData {
      FMOD::Sound* sound = nullptr;
      bool is_stream = false; // true for music/long audio, false for sound effects

      SoundData(const std::string& path, bool stream = false);
      ~SoundData();

      // Delete copy constructor and assignment operator
      SoundData(const SoundData&) = delete;
      SoundData& operator=(const SoundData&) = delete;
      SoundData(const SoundData&&) = delete;

      // Default constructor
      SoundData();
    };

  } // namespace Audio
} // namespace Engine