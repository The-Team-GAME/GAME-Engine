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

    FMOD::System* get_fmod_system() {
      static UniqueFMODSystem fmod_system = []() -> UniqueFMODSystem {
        FMOD::System* system = nullptr;
        FMOD_RESULT result = FMOD::System_Create(&system);

        if (result != FMOD_OK) {
          Log::error("Failed to create FMOD system");
          return nullptr;
        }

        // Initialize with 512 channels, FMOD_INIT_NORMAL, and no extra driver data
        result = system->init(512, FMOD_INIT_NORMAL, nullptr);
        if (result != FMOD_OK) {
          Log::error("Failed to initialize FMOD system");
          system->release();
          return nullptr;
        }

        Log::info("FMOD system initialized");
        return UniqueFMODSystem(system);
        }();

      if (!fmod_system) {
        throw std::runtime_error("FMOD system failed to initialize");
      }

      return fmod_system.get();
    }

    // SoundData default constructor
    SoundData::SoundData() : sound(nullptr), is_stream(false) {}

    // SoundData constructor
    SoundData::SoundData(const std::string& path, bool stream) : is_stream(stream) {
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
        Log::warning("Failed to load sound: " + path);
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