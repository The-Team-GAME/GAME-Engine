#include "AudioPlayer.h"
#include "AudioPlayerInternal.h"
#include "../Logger.h"
#include <fmod.hpp>

namespace Engine {
  namespace Audio {

    // Listener state
    Math::Vec2D listener_position = { 0.0f, 0.0f };
    Math::Vec2D listener_forward = { 0.0f, 1.0f };
    f32 master_volume = 1.0f;

    bool init() {
      // FMOD system is initialized lazily in get_fmod_system()
      // Just verify it works
      try {
        FMOD::System* system = get_fmod_system();
        if (!system) {
          Log::error("Failed to initialize audio system");
          return false;
        }
        Log::info("Audio system initialized");
        return true;
      }
      catch (const std::exception& e) {
        Log::error("Audio initialization failed: " + std::string(e.what()));
        return false;
      }
    }

    bool exit() {
      // FMOD system cleanup is handled by UniqueFMODSystem destructor
      Log::info("Audio system cleaned up");
      return true;
    }

    void update() {
      // Update FMOD audio system every frame
      FMOD::System* fmod_system = Engine::Audio::get_fmod_system();
      if (fmod_system) {
        fmod_system->update();
      }
    }

    ChannelHandle play_sound(Sound sound, f32 volume, bool loop) {
      if (!sound || !sound->sound) {
        Log::warning("Attempted to play invalid sound");
        return INVALID_CHANNEL;
      }

      FMOD::System* system = get_fmod_system();
      FMOD::Channel* channel = nullptr;

      FMOD_RESULT result = system->playSound(sound->sound, nullptr, true, &channel);
      if (result != FMOD_OK || !channel) {
        Log::warning("Failed to play sound");
        return INVALID_CHANNEL;
      }

      // Set loop mode
      channel->setMode(loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

      // Set volume (clamped to master volume)
      channel->setVolume(volume * master_volume);

      // Set as 2D sound (no positional audio)
      channel->setMode(FMOD_2D);

      // Unpause and start playing
      channel->setPaused(false);

      // Get channel index to use as handle
      int index;
      channel->getIndex(&index);
      return static_cast<ChannelHandle>(index);
    }

    ChannelHandle play_sound_at(Sound sound, const Math::Vec2D& position, f32 volume, bool loop) {
      if (!sound || !sound->sound) {
        Log::warning("Attempted to play invalid sound");
        return INVALID_CHANNEL;
      }

      FMOD::System* system = get_fmod_system();
      FMOD::Channel* channel = nullptr;

      FMOD_RESULT result = system->playSound(sound->sound, nullptr, true, &channel);
      if (result != FMOD_OK || !channel) {
        Log::warning("Failed to play sound");
        return INVALID_CHANNEL;
      }

      // Set loop mode
      channel->setMode(loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

      // Set volume
      channel->setVolume(volume * master_volume);

      // Set as 3D sound
      channel->setMode(FMOD_3D);

      // Set position
      FMOD_VECTOR pos = { position.x, position.y, 0.0f };
      FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
      channel->set3DAttributes(&pos, &vel);

      // Set 3D settings (adjust these based on your game's scale)
      channel->set3DMinMaxDistance(1.0f, 10000.0f);

      // Unpause and start playing
      channel->setPaused(false);

      // Get channel index
      int index;
      channel->getIndex(&index);
      return static_cast<ChannelHandle>(index);
    }

    void stop_channel(ChannelHandle handle) {
      if (handle == INVALID_CHANNEL) return;

      FMOD::System* system = get_fmod_system();
      FMOD::Channel* channel = nullptr;

      if (system->getChannel(handle, &channel) == FMOD_OK && channel) {
        channel->stop();
      }
    }

    void pause_channel(ChannelHandle handle) {
      if (handle == INVALID_CHANNEL) return;

      FMOD::System* system = get_fmod_system();
      FMOD::Channel* channel = nullptr;

      if (system->getChannel(handle, &channel) == FMOD_OK && channel) {
        channel->setPaused(true);
      }
    }

    void resume_channel(ChannelHandle handle) {
      if (handle == INVALID_CHANNEL) return;

      FMOD::System* system = get_fmod_system();
      FMOD::Channel* channel = nullptr;

      if (system->getChannel(handle, &channel) == FMOD_OK && channel) {
        channel->setPaused(false);
      }
    }

    void set_channel_volume(ChannelHandle handle, f32 volume) {
      if (handle == INVALID_CHANNEL) return;

      FMOD::System* system = get_fmod_system();
      FMOD::Channel* channel = nullptr;

      if (system->getChannel(handle, &channel) == FMOD_OK && channel) {
        channel->setVolume(volume * master_volume);
      }
    }

    void set_channel_position(ChannelHandle handle, const Math::Vec2D& position) {
      if (handle == INVALID_CHANNEL) return;

      FMOD::System* system = get_fmod_system();
      FMOD::Channel* channel = nullptr;

      if (system->getChannel(handle, &channel) == FMOD_OK && channel) {
        FMOD_VECTOR pos = { position.x, position.y, 0.0f };
        FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
        channel->set3DAttributes(&pos, &vel);
      }
    }

    bool is_channel_playing(ChannelHandle handle) {
      if (handle == INVALID_CHANNEL) return false;

      FMOD::System* system = get_fmod_system();
      FMOD::Channel* channel = nullptr;

      if (system->getChannel(handle, &channel) == FMOD_OK && channel) {
        bool playing = false;
        channel->isPlaying(&playing);
        return playing;
      }

      return false;
    }

    void set_master_volume(f32 volume) {
      master_volume = glm::clamp(volume, 0.0f, 1.0f);
    }

    void set_listener_position(const Math::Vec2D& position) {
      listener_position = position;
      
      FMOD::System* system = get_fmod_system();
      FMOD_VECTOR pos = { position.x, position.y, 0.0f };
      FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
      FMOD_VECTOR forward = { listener_forward.x, listener_forward.y, 0.0f };
      FMOD_VECTOR up = { 0.0f, 0.0f, 1.0f };
      
      system->set3DListenerAttributes(0, &pos, &vel, &forward, &up);
    }

    void set_listener_forward(const Math::Vec2D& forward) {
      listener_forward = forward;
      
      FMOD::System* system = get_fmod_system();
      FMOD_VECTOR pos = { listener_position.x, listener_position.y, 0.0f };
      FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
      FMOD_VECTOR fwd = { forward.x, forward.y, 0.0f };
      FMOD_VECTOR up = { 0.0f, 0.0f, 1.0f };
      
      system->set3DListenerAttributes(0, &pos, &vel, &fwd, &up);
    }

    f32 get_master_volume() {
      return master_volume;
    }

    Math::Vec2D get_listener_position() {
      return listener_position;
    }

    void stop_all_sounds() {
      FMOD::System* system = get_fmod_system();
      
      // Get master channel group
      FMOD::ChannelGroup* master_group = nullptr;
      if (system->getMasterChannelGroup(&master_group) == FMOD_OK && master_group) {
        master_group->stop();
      }
    }

    void pause_all_sounds() {
      FMOD::System* system = get_fmod_system();
      
      FMOD::ChannelGroup* master_group = nullptr;
      if (system->getMasterChannelGroup(&master_group) == FMOD_OK && master_group) {
        master_group->setPaused(true);
      }
    }

    void resume_all_sounds() {
      FMOD::System* system = get_fmod_system();
      
      FMOD::ChannelGroup* master_group = nullptr;
      if (system->getMasterChannelGroup(&master_group) == FMOD_OK && master_group) {
        master_group->setPaused(false);
      }
    }

  } // namespace Audio
} // namespace Engine