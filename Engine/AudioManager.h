//-----------------------------------------------------------------------------------
// file:    AudioManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the AudioManager class which provides an interface
//          for loading, playing, and managing audio using the FMOD audio library.
//          Supports sound loading, playback control, volume management, and looping.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
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
    FMOD::System* fmodSystem; // FMOD system instance
    std::unordered_map<s32, FMOD::Sound*> sounds; // Map of sound IDs to FMOD sound objects
    std::unordered_map<s32, FMOD::Channel*> channels; // Map of sound IDs to active channels
    s32 nextSoundId; // Counter for generating unique sound IDs

  public:
    AudioManager(); // Constructor - initializes FMOD system
    ~AudioManager(); // Destructor - cleans up all sounds and channels

    /* ============================== //
    // Sound loading and management   //
    // ============================== */
    s32 loadSound(const std::string& filepath); // Loads a sound file and returns its unique ID
    void unloadSound(s32 soundId); // Unloads a specific sound by ID
    void unloadAllSounds(); // Unloads all loaded sounds

    /* ================== //
    // Playback control   //
    // ================== */
    bool playSound(s32 soundId); // Plays a sound once by ID
    bool playSoundLooped(s32 soundId); // Plays a sound in loop mode by ID
    void stopSound(s32 soundId); // Stops a specific sound by ID
    void stopAllSounds(); // Stops all currently playing sounds

    /* ================ //
    // Volume control   //
    // ================ */
    void setVolume(s32 soundId, f32 volume); // Sets volume for a specific sound (0.0 to 1.0)
    void setMasterVolume(f32 volume); // Sets master volume for all sounds (0.0 to 1.0)

    /* ======= //
    // Update  //
    // ======= */
    void update(); // Updates FMOD system and cleans up finished channels (call every frame)

    /* ========= //
    // Utility   //
    // ========= */
    bool isPlaying(s32 soundId); // Checks if a sound is currently playing
  };
}
