#pragma once

#include "Precompiled.h"
#include <string>
#include <unordered_map>
#include "TypeDef.h"
namespace Engine
{
  class AudioManager
  {
  private:
    FMOD::System* fmodSystem;
    std::unordered_map<s32, FMOD::Sound*> sounds;
    std::unordered_map<s32, FMOD::Channel*> channels;
    s32 nextSoundId;

  public:
    AudioManager();
    ~AudioManager();

    // Sound loading and management
    s32 loadSound(const std::string& filepath);
    void unloadSound(s32 soundId);
    void unloadAllSounds();

    // Playback control
    bool playSound(s32 soundId);
    bool playSoundLooped(s32 soundId);
    void stopSound(s32 soundId);
    void stopAllSounds();

    // Volume control
    void setVolume(s32 soundId, f32 volume);
    void setMasterVolume(f32 volume);

    // Update (call this every frame)
    void update();

    // Utility
    bool isPlaying(s32 soundId);
  };
}
