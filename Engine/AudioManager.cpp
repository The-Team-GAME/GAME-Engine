#include "AudioManager.h"
#include <iostream>
#include <fmod_errors.h>
#include "Core.h"

namespace Engine
{
  AudioManager::AudioManager() : fmodSystem(nullptr), nextSoundId(1)
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

  int AudioManager::loadSound(const std::string& filepath)
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

    int soundId = nextSoundId++;
    sounds[soundId] = sound;
    std::cout << "AudioManager: Successfully loaded sound '" << filepath << "' with ID " << soundId << std::endl;
    return soundId;
  }

  void AudioManager::unloadSound(int soundId)
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

  bool AudioManager::playSound(int soundId)
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

  bool AudioManager::playSoundLooped(int soundId)
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

  void AudioManager::stopSound(int soundId)
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

  void AudioManager::setVolume(int soundId, float volume)
  {
    auto it = channels.find(soundId);
    if (it != channels.end())
    {
      it->second->setVolume(volume);
    }
  }

  void AudioManager::setMasterVolume(float volume)
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

  void AudioManager::update()
  {
    PERFORMANCE.start("Audio");

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
    
    PERFORMANCE.stop("Audio");
  }

  bool AudioManager::isPlaying(int soundId)
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