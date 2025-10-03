//-----------------------------------------------------------------------------------
// file:    AudioManager.cpp
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   This file implements the AudioManager class which provides an interface
//          for loading, playing, and managing audio using the FMOD audio library.
//          Supports sound loading, playback control, volume management, and looping.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "AudioManager.h"
#include <iostream>
#include <fmod_errors.h>
#include "TypeDef.h"
#include "Core.h"

namespace Engine
{
  /*=================================================================================
  Function Brief:
  Constructor that initializes the FMOD system for audio playback.
  Creates and initializes an FMOD system instance with 512 virtual channels.
  ==================================================================================*/
  AudioManager::AudioManager() : fmodSystem(nullptr), next_sound_id(1)
  {
    FMOD_RESULT result;

    // Create FMOD system
    result = FMOD::System_Create(&fmodSystem);
    if (result != FMOD_OK)
    {
      std::cout << "AudioManager: Failed to create FMOD system! " << FMOD_ErrorString(result) << std::endl;
    }

    // Initialize FMOD system
    result = fmodSystem->init(512, FMOD_INIT_NORMAL, 0);
    if (result != FMOD_OK)
    {
      std::cout << "AudioManager: Failed to initialize FMOD system! " << FMOD_ErrorString(result) << std::endl;
    }

    std::cout << "AudioManager: Successfully initialized FMOD system" << std::endl;
  }

  /*=================================================================================
  Function Brief:
  Destructor that cleans up all audio resources and shuts down the FMOD system.
  Stops all playing sounds, releases all loaded sounds, and closes the FMOD system.
  ==================================================================================*/
  AudioManager::~AudioManager()
  {
    // Stop all sounds
    stopAllSounds();

    // Release all loaded sounds
    unloadAllSounds();

    // Shutdown FMOD system
    if (fmodSystem)
    {
      fmodSystem->close();
      fmodSystem->release();
      fmodSystem = nullptr;
    }
  }

  /*=================================================================================
  Function Brief:
  Loads an audio file from the specified filepath and assigns it a unique ID.
  Returns the sound ID on success, or -1 if loading fails.

  Parameters:
  filepath - The path to the audio file to load

  Returns:
  s32 - Unique sound ID on success, -1 on failure
  ==================================================================================*/
  s32 AudioManager::loadSound(const std::string& filepath)
  {
    if (!fmodSystem)
    {
      std::cout << "AudioManager: FMOD system not initialized!" << std::endl;
      return -1;
    }

    FMOD::Sound* sound = nullptr;
    FMOD_RESULT result = fmodSystem->createSound(filepath.c_str(), FMOD_DEFAULT, 0, &sound);

    if (result != FMOD_OK)
    {
      std::cout << "AudioManager: Failed to load sound '" << filepath << "'! " << FMOD_ErrorString(result) << std::endl;
      return -1;
    }

    s32 soundId = next_sound_id++;
    sounds[soundId] = sound;
    std::cout << "AudioManager: Successfully loaded sound '" << filepath << "' with ID " << soundId << std::endl;
    return soundId;
  }

  /*=================================================================================
  Function Brief:
  Unloads a sound from memory by its ID. Stops the sound if it's currently playing
  before releasing its resources.

  Parameters:
  soundId - The ID of the sound to unload
  ==================================================================================*/
  void AudioManager::unloadSound(s32 soundId)
  {
    // Stop the sound if it's playing
    stopSound(soundId);

    auto it = sounds.find(soundId);
    if (it != sounds.end())
    {
      it->second->release();
      sounds.erase(it);
      std::cout << "AudioManager: Unloaded sound with ID " << soundId << std::endl;
    }
  }

  /*=================================================================================
  Function Brief:
  Unloads all sounds from memory and clears all active channels.
  Releases all FMOD sound resources.
  ==================================================================================*/
  void AudioManager::unloadAllSounds()
  {
    for (auto& pair : sounds)
    {
      pair.second->release();
    }
    sounds.clear();
    channels.clear();
    std::cout << "AudioManager: Unloaded all sounds" << std::endl;
  }

  /*=================================================================================
  Function Brief:
  Plays a sound once by its ID. If the sound is already playing, stops the existing
  instance before starting a new one.

  Parameters:
  soundId - The ID of the sound to play

  Returns:
  bool - true if playback started successfully, false otherwise
  ==================================================================================*/
  bool AudioManager::playSound(s32 soundId)
  {
    if (!fmodSystem)
    {
      std::cout << "AudioManager: FMOD system not initialized!" << std::endl;
      return false;
    }

    auto it = sounds.find(soundId);
    if (it == sounds.end())
    {
      std::cout << "AudioManager: Sound with ID " << soundId << " not found!" << std::endl;
      return false;
    }

    // Stop existing channel if sound is already playing
    stopSound(soundId);

    FMOD::Channel* channel = nullptr;
    FMOD_RESULT result = fmodSystem->playSound(it->second, 0, false, &channel);

    if (result != FMOD_OK)
    {
      std::cout << "AudioManager: Failed to play sound with ID " << soundId << "! " << FMOD_ErrorString(result) << std::endl;
      return false;
    }

    // Store channel for later control
    channels[soundId] = channel;

    return true;
  }

  /*=================================================================================
  Function Brief:
  Plays a sound in loop mode by its ID. The sound will continuously repeat until
  manually stopped. If the sound is already playing, stops the existing instance
  before starting a new one.

  Parameters:
  soundId - The ID of the sound to play in loop mode

  Returns:
  bool - true if playback started successfully, false otherwise
  ==================================================================================*/
  bool AudioManager::playSoundLooped(s32 soundId)
  {
    if (!fmodSystem)
    {
      std::cout << "AudioManager: FMOD system not initialized!" << std::endl;
      return false;
    }

    auto it = sounds.find(soundId);
    if (it == sounds.end())
    {
      std::cout << "AudioManager: Sound with ID " << soundId << " not found!" << std::endl;
      return false;
    }

    // Stop existing channel if sound is already playing
    stopSound(soundId);

    // Set sound to loop
    it->second->setMode(FMOD_LOOP_NORMAL);

    FMOD::Channel* channel = nullptr;
    FMOD_RESULT result = fmodSystem->playSound(it->second, 0, false, &channel);

    if (result != FMOD_OK)
    {
      std::cout << "AudioManager: Failed to play looped sound with ID " << soundId << "! " << FMOD_ErrorString(result) << std::endl;
      return false;
    }

    // Store channel for later control
    channels[soundId] = channel;

    return true;
  }

  /*=================================================================================
  Function Brief:
  Stops a currently playing sound by its ID and removes it from the active channels.

  Parameters:
  soundId - The ID of the sound to stop
  ==================================================================================*/
  void AudioManager::stopSound(s32 soundId)
  {
    auto it = channels.find(soundId);
    if (it != channels.end())
    {
      bool isPlaying = false;
      it->second->isPlaying(&isPlaying);
      if (isPlaying)
      {
        it->second->stop();
      }
      channels.erase(it);
    }
  }

  /*=================================================================================
  Function Brief:
  Stops all currently playing sounds and clears all active channels.
  ==================================================================================*/
  void AudioManager::stopAllSounds()
  {
    for (auto& pair : channels)
    {
      bool isPlaying = false;
      pair.second->isPlaying(&isPlaying);
      if (isPlaying)
      {
        pair.second->stop();
      }
    }
    channels.clear();
  }

  /*=================================================================================
  Function Brief:
  Sets the volume for a specific sound by its ID. Volume range is 0.0 (silent) to
  1.0 (full volume).

  Parameters:
  soundId - The ID of the sound to adjust
  volume - The volume level (0.0 to 1.0)
  ==================================================================================*/
  void AudioManager::setVolume(s32 soundId, f32 volume)
  {
    auto it = channels.find(soundId);
    if (it != channels.end())
    {
      it->second->setVolume(volume);
    }
  }

  /*=================================================================================
  Function Brief:
  Sets the master volume for all sounds. Affects all currently playing and future
  sounds. Volume range is 0.0 (silent) to 1.0 (full volume).

  Parameters:
  volume - The master volume level (0.0 to 1.0)
  ==================================================================================*/
  void AudioManager::setMasterVolume(f32 volume)
  {
    if (fmodSystem)
    {
      FMOD::ChannelGroup* masterGroup = nullptr;
      fmodSystem->getMasterChannelGroup(&masterGroup);
      if (masterGroup)
      {
        masterGroup->setVolume(volume);
      }
    }
  }

  /*=================================================================================
  Function Brief:
  Updates the FMOD system and cleans up finished audio channels. This function
  should be called every frame to ensure proper audio playback and resource management.

  Where to call it:
  Call this function in the main game loop, typically once per frame.
  ==================================================================================*/
  void AudioManager::update()
  {
    Engine::PERFORMANCE.start("Audio");

    if (fmodSystem)
    {
      fmodSystem->update();

      // Clean up finished channels
      auto it = channels.begin();
      while (it != channels.end())
      {
        bool isPlaying = false;
        it->second->isPlaying(&isPlaying);
        if (!isPlaying)
        {
          it = channels.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }

    Engine::PERFORMANCE.stop("Audio");
  }

  /*=================================================================================
  Function Brief:
  Checks if a sound is currently playing by its ID.

  Parameters:
  soundId - The ID of the sound to check

  Returns:
  bool - true if the sound is playing, false otherwise
  ==================================================================================*/
  bool AudioManager::isPlaying(s32 soundId)
  {
    auto it = channels.find(soundId);
    if (it != channels.end())
    {
      bool isPlaying = false;
      it->second->isPlaying(&isPlaying);
      return isPlaying;
    }
    return false;
  }
}