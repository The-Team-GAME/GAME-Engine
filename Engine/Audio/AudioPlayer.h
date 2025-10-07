#pragma once

#include "../Vector2D.h"
#include "AudioInternal.h"
#include <memory>
#include <string>

namespace Engine {
  namespace Audio {

    using Sound = std::shared_ptr<SoundData>;

    // Initialization
    bool init();

    // Cleanup
    bool exit();

    // Update FMOD system (call this every frame)
    void update();

    // Play a sound effect (non-looping by default)
    void play_sound(Sound sound, float volume = 1.0f, bool loop = false);

    // Play music (looping by default)
    void play_music(Sound music, float volume = 1.0f, bool loop = true);

    // Stop a specific sound
    void stop_sound(Sound sound);

    // Stop all sounds
    void stop_all_sounds();

    // Set master volume (0.0 to 1.0)
    void set_master_volume(float volume);

    // Get master volume
    float get_master_volume();

  } // namespace Audio
} // namespace Engine