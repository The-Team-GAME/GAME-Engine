//-----------------------------------------------------------------------------------
// file:    AudioPlayerInternal.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Internal implementation details of the FMOD audio system.
//          Handles FMOD system initialization, channel management, and low-level
//          audio operations. Provides the foundation for the public audio APIs
//          with proper resource management and error handling.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "AudioPlayerInternal.h"
#include <fmod.hpp>
#include <stdexcept>

namespace Engine {
  namespace Audio {

    struct FMODDeleter {
      void operator()(FMOD::System* system) const {
        if (system) {
          system->release();
        }
      }
    };

    using UniqueFMODSystem = std::unique_ptr<FMOD::System, FMODDeleter>;

    struct FMODSystemWrapper {
      FMOD::System* system = nullptr;

      FMODSystemWrapper() {
        FMOD_RESULT result = FMOD::System_Create(&system);
        if (result != FMOD_OK) {
          Log::error("Failed to create FMOD system");
          system = nullptr;
          return;
        }

        // Initialize with 512 channels, FMOD_INIT_NORMAL, and no extra driver data
        result = system->init(512, FMOD_INIT_NORMAL, nullptr);
        if (result != FMOD_OK) {
          Log::error("Failed to initialize FMOD system");
          system->release();
          system = nullptr;
          return;
        }
        Log::info("FMOD system initialized");
      }

      ~FMODSystemWrapper() {
        if (system) {
          system->release();
          Log::info("FMOD system released");
        }
      }
    };

    // Global shared pointer to the system wrapper
    // This ensures the system stays alive as long as anyone (SoundData) holds a reference
    std::shared_ptr<FMODSystemWrapper> global_fmod_system;

    std::shared_ptr<void> get_fmod_system_ref() {
      if (!global_fmod_system) {
        global_fmod_system = std::make_shared<FMODSystemWrapper>();
      }
      return global_fmod_system;
    }

    FMOD::System* get_fmod_system() {
      if (!global_fmod_system) {
        global_fmod_system = std::make_shared<FMODSystemWrapper>();
      }
      return global_fmod_system->system;
    }

    void release_fmod_system() {
      // Reset the shared_ptr so FMODSystemWrapper::~FMODSystemWrapper() runs now,
      // calling system->release() and freeing all FMOD internal allocations
      // before the CRT leak check fires at process exit.
      global_fmod_system.reset();
    }

    // SoundData default constructor
    SoundData::SoundData() : sound(nullptr), is_stream(false) {}

    // SoundData constructor
    SoundData::SoundData(const std::string& path, bool stream) : is_stream(stream), path(path) {
      system_ref = get_fmod_system_ref(); // Keep system alive
      FMOD::System* system = get_fmod_system();

      FMOD_MODE mode = FMOD_DEFAULT;
      if (stream) {
        mode |= FMOD_CREATESTREAM; // Stream from disk (for music)
      }
      else {
        mode |= FMOD_CREATESAMPLE; // Load into memory (for sound effects)
      }

      // Add 3D support
      mode |= FMOD_3D;

      FMOD_RESULT result = system->createSound(path.c_str(), mode, nullptr, &sound);

      if (result != FMOD_OK) {
        Log::warning("Failed to load sound: {}", path);
        sound = nullptr;
      }
    }

    // SoundData destructor
    SoundData::~SoundData() {
      if (sound) {
        sound->release();
        sound = nullptr;
      }
    }

  } // namespace Audio
} // namespace Engine