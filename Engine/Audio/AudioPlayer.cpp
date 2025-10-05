#include "AudioPlayer.h"
#include "../Logger.h"
#include <vector>

namespace Engine {
  namespace Audio {

    static FMOD::ChannelGroup* master_channel_group = nullptr;
    static std::vector<FMOD::Channel*> active_channels;

    bool init() {
      try {
        FMOD::System* system = get_fmod_system();

        // Create master channel group
        FMOD_RESULT result = system->getMasterChannelGroup(&master_channel_group);
        if (result != FMOD_OK) {
          Log::error("Failed to get master channel group");
          return false;
        }

        Log::info("Audio manager initialized");
        return true;
      }
      catch (const std::exception& e) {
        Log::error(std::string("Audio initialization failed: ") + e.what());
        return false;
      }
    }

    bool exit() {
      stop_all_sounds();
      active_channels.clear();
      master_channel_group = nullptr;
      Log::info("Audio manager cleaned up");
      return true;
    }

    void update() {
      FMOD::System* system = get_fmod_system();
      system->update();

      // Clean up finished channels
      active_channels.erase(
        std::remove_if(active_channels.begin(), active_channels.end(),
          [](FMOD::Channel* channel) {
            bool isPlaying = false;
            if (channel) {
              channel->isPlaying(&isPlaying);
            }
            return !isPlaying;
          }),
        active_channels.end()
      );
    }

    void play_sound(Sound sound, float volume, bool loop) {
      if (!sound || !sound->sound) {
        Log::warning("Attempted to play invalid sound");
        return;
      }

      FMOD::System* system = get_fmod_system();
      FMOD::Channel* channel = nullptr;

      FMOD_RESULT result = system->playSound(sound->sound, nullptr, false, &channel);

      if (result != FMOD_OK || !channel) {
        Log::warning("Failed to play sound");
        return;
      }

      channel->setVolume(volume);
      channel->setMode(loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

      active_channels.push_back(channel);
    }

    void play_music(Sound music, float volume, bool loop) {
      play_sound(music, volume, loop);
    }

    void stop_all_sounds() {
      for (FMOD::Channel* channel : active_channels) {
        if (channel) {
          channel->stop();
        }
      }
      active_channels.clear();
    }

    void set_master_volume(float volume) {
      if (master_channel_group) {
        master_channel_group->setVolume(volume);
      }
    }

    float get_master_volume() {
      if (master_channel_group) {
        float volume = 1.0f;
        master_channel_group->getVolume(&volume);
        return volume;
      }
      return 1.0f;
    }

  } // namespace Audio
} // namespace Engine