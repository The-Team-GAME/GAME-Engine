#include "AudioInternal.h"
#include <stdexcept>

namespace Engine {
  namespace Audio {

    struct FMODSystemDeleter {
      void operator()(FMOD::System* system) const {
        if (system) {
          system->close();
          system->release();
        }
      }
    };

    using UniqueFMODSystem = std::unique_ptr<FMOD::System, FMODSystemDeleter>;

    FMOD::System* get_fmod_system() {
      static UniqueFMODSystem fmod_system = []() -> UniqueFMODSystem {
        FMOD::System* system = nullptr;
        FMOD_RESULT result = FMOD::System_Create(&system);

        if (result != FMOD_OK) {
          Log::error("FMOD: Failed to create system");
          return nullptr;
        }

        result = system->init(512, FMOD_INIT_NORMAL, nullptr);
        if (result != FMOD_OK) {
          Log::error("FMOD: Failed to initialize system");
          system->release();
          return nullptr;
        }

        Log::info("FMOD system initialized");
        return UniqueFMODSystem(system);
        }();

      if (!fmod_system) {
        throw std::runtime_error("FMOD system failed to initialize.");
      }

      return fmod_system.get();
    }

    // SoundData constructor
    SoundData::SoundData(const std::string& path, bool stream)
      : is_stream(stream) {
      FMOD::System* system = get_fmod_system();

      FMOD_MODE mode = FMOD_DEFAULT;
      if (stream) {
        mode |= FMOD_CREATESTREAM;
      }
      else {
        mode |= FMOD_CREATESAMPLE;
      }

      FMOD_RESULT result = system->createSound(path.c_str(), mode, nullptr, &sound);

      if (result != FMOD_OK) {
        Log::warning("Failed to load audio: " + path);
        sound = nullptr;
        return;
      }

      Log::info("Audio loaded: " + path);
    }

    // SoundData default constructor
    SoundData::SoundData() : sound(nullptr), is_stream(false) {}

    // SoundData destructor
    SoundData::~SoundData() {
      if (sound) {
        sound->release();
      }
    }

  } // namespace Audio
} // namespace Engine