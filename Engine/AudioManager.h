#pragma once

#include "Precompiled.h"
#include <string>
#include <unordered_map>

namespace Engine
{
  class AudioManager
  {
  private:
    FMOD::System* fmodSystem;
    std::unordered_map<int, FMOD::Sound*> sounds;
    std::unordered_map<std::string, FMOD::Channel*> channels;
    int nextSoundId;

  public:
    AudioManager();
    ~AudioManager();

    // Sound loading and management
    int loadSound(const std::string& filepath);
    void unloadSound(int soundId);
    void unloadAllSounds();

    // Playback control
    bool playSound(int soundId, const std::string& channelName = "");
    bool playSoundLooped(int soundId, const std::string& channelName = "");
    void stopSound(const std::string& channelName);
    void stopAllSounds();

    // Volume control
    void setVolume(const std::string& channelName, float volume);
    void setMasterVolume(float volume);

    // Update (call this every frame)
    void update();

    // Utility
    bool isPlaying(const std::string& channelName);
  };
}
