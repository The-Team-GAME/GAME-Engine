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
    std::unordered_map<int, FMOD::Channel*> channels;
    int nextSoundId;

  public:
    AudioManager();
    ~AudioManager();

    // Sound loading and management
    int loadSound(const std::string& filepath);
    void unloadSound(int soundId);
    void unloadAllSounds();

    // Playback control
    bool playSound(int soundId);
    bool playSoundLooped(int soundId);
    void stopSound(int soundId);
    void stopAllSounds();

    // Volume control
    void setVolume(int soundId, float volume);
    void setMasterVolume(float volume);

    // Update (call this every frame)
    void update();

    // Utility
    bool isPlaying(int soundId);
  };
}
