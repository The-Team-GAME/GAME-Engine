//-----------------------------------------------------------------------------------
// file:    AudioSystemExample.cpp
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   Example usage and integration guide for the AudioSystem.
//          Demonstrates initialization, playback, 3D audio, event instances,
//          parameters, and volume controls for FMOD Studio integration.
//
// Copyright © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "AudioSystem.h"

// ==============================================================
// INITIALIZATION (In your main init function or Core::init())
// ==============================================================
void InitializeAudio()
{
    // Initialize the audio system
    if (!Engine::AudioSystem::init()) {
        // Handle error
        return;
    }

    // Load your FMOD Studio banks
    // Make sure these files exist in your Assets/sounds folder!
    Engine::AudioSystem::loadBank("Assets/sounds/Master.bank");
    Engine::AudioSystem::loadBank("Assets/sounds/Master.strings.bank");
    //Engine::AudioSystem::loadBank("Assets/sounds/Music.bank");
    Engine::AudioSystem::loadBank("Assets/sounds/SFX.bank");
}

// ==============================================================
// UPDATE (In your game loop - already done in Core.cpp!)
// ==============================================================
void UpdateAudio()
{
    // This is already integrated into Core::run()
    Engine::AudioSystem::update();
}

// ==============================================================
// SHUTDOWN (In your cleanup function or Core::exit())
// ==============================================================
void ShutdownAudio()
{
    // Already integrated into Core::exit()
    Engine::AudioSystem::shutdown();
}

// ==============================================================
// SIMPLE SOUND EFFECTS (One-shot sounds)
// ==============================================================
void PlayHurtEffect()
{
    // UI sounds
    Engine::AudioSystem::playEvent("event:/SFX/ButtonClick");
    Engine::AudioSystem::playEvent("event:/UI/ButtonHover");
    
    // Game sounds
    Engine::AudioSystem::playEvent("event:/Player/Jump");
    Engine::AudioSystem::playEvent("event:/Player/Footstep");
    Engine::AudioSystem::playEvent("event:/Weapon/Shoot");
    
    // Play at a specific 2D position (spatial audio)
    Math::Vec2D explosionPos(500.0f, 300.0f);
    Engine::AudioSystem::playEventAtPosition("event:/SFX/Explosion", explosionPos);
}

// ==============================================================
// BACKGROUND MUSIC (Persistent instance)
// ==============================================================
class MusicController
{
private:
    bool musicPlaying = false;

public:
    void PlayMainMenu()
    {
        StopMusic(); // Stop any existing music
        
        Engine::AudioSystem::createEventInstance("bgm", "event:/Music/MainMenu");
        Engine::AudioSystem::startEventInstance("bgm");
        musicPlaying = true;
    }
    
    void PlayGameMusic()
    {
        StopMusic();
        
        Engine::AudioSystem::createEventInstance("bgm", "event:/Music/Gameplay");
        Engine::AudioSystem::startEventInstance("bgm");
        musicPlaying = true;
    }
    
    void PauseMusic()
    {
        if (musicPlaying) {
            Engine::AudioSystem::pauseEventInstance("bgm", true);
        }
    }
    
    void ResumeMusic()
    {
        if (musicPlaying) {
            Engine::AudioSystem::pauseEventInstance("bgm", false);
        }
    }
    
    void StopMusic()
    {
        if (musicPlaying) {
            Engine::AudioSystem::stopEventInstance("bgm");
            Engine::AudioSystem::releaseEventInstance("bgm");
            musicPlaying = false;
        }
    }
    
    void SetMusicVolume(float volume) // 0.0 to 1.0
    {
        if (musicPlaying) {
            Engine::AudioSystem::setEventInstanceVolume("bgm", volume);
        }
    }
};

// ==============================================================
// LOOPING SOUNDS (Footsteps, engine sounds, etc.)
// ==============================================================
class PlayerSoundController
{
private:
    Math::Vec2D playerPosition;
    bool footstepsPlaying = false;

public:
    void SetPosition(const Math::Vec2D& pos)
    {
        playerPosition = pos;
    }

    void StartFootsteps()
    {
        if (!footstepsPlaying) {
            Engine::AudioSystem::createEventInstance("footsteps", "event:/Player/Footsteps");
            Engine::AudioSystem::setEventInstancePosition("footsteps", playerPosition);
            Engine::AudioSystem::startEventInstance("footsteps");
            footstepsPlaying = true;
        }
    }
    
    void StopFootsteps()
    {
        if (footstepsPlaying) {
            Engine::AudioSystem::stopEventInstance("footsteps");
            Engine::AudioSystem::releaseEventInstance("footsteps");
            footstepsPlaying = false;
        }
    }
    
    void Update()
    {
        // Update 3D position every frame
        if (footstepsPlaying) {
            Engine::AudioSystem::setEventInstancePosition("footsteps", playerPosition);
        }
    }
    
    void Jump()
    {
        // One-shot sounds don't need to be managed
        Engine::AudioSystem::playEventAtPosition("event:/Player/Jump", playerPosition);
    }
    
    void Land()
    {
        Engine::AudioSystem::playEventAtPosition("event:/Player/Land", playerPosition);
    }
};

// ==============================================================
// DYNAMIC MUSIC (With parameters)
// ==============================================================
class DynamicMusicController
{
public:
    void StartDynamicMusic()
    {
        Engine::AudioSystem::createEventInstance("dynamic_music", "event:/Music/DynamicGameplay");
        Engine::AudioSystem::startEventInstance("dynamic_music");
    }
    
    void SetIntensity(float intensity) // 0.0 to 10.0
    {
        // This parameter must be defined in FMOD Studio
        Engine::AudioSystem::setEventParameter("dynamic_music", "Intensity", intensity);
    }
    
    void SetCombatState(bool inCombat)
    {
        float combatValue = inCombat ? 1.0f : 0.0f;
        Engine::AudioSystem::setEventParameter("dynamic_music", "Combat", combatValue);
    }
    
    void Stop()
    {
        Engine::AudioSystem::stopEventInstance("dynamic_music");
        Engine::AudioSystem::releaseEventInstance("dynamic_music");
    }
};

// ==============================================================
// 3D AUDIO WITH LISTENER (Camera)
// ==============================================================
class AudioListener
{
public:
    void UpdateCameraPosition(const Math::Vec2D& cameraPos)
    {
        // Simple listener update
        Engine::AudioSystem::setListenerPosition(cameraPos);
    }
    
    void UpdateCameraFull(const Math::Vec2D& pos, const Math::Vec2D& velocity)
    {
        // Full listener attributes for Doppler effect
        Math::Vec2D forward(0.0f, 1.0f);
        Math::Vec2D up(0.0f, 0.0f);
        Engine::AudioSystem::setListenerAttributes(pos, velocity, forward, up);
    }
};

// ==============================================================
// VOLUME CONTROLS (Settings menu)
// ==============================================================
class AudioSettings
{
public:
    void SetMasterVolume(float volume) // 0.0 to 1.0
    {
        Engine::AudioSystem::setMasterVolume(volume);
    }
    
    void SetMusicVolume(float volume)
    {
        Engine::AudioSystem::setBusVolume("bus:/Music", volume);
    }
    
    void SetSFXVolume(float volume)
    {
        Engine::AudioSystem::setBusVolume("bus:/SFX", volume);
    }
    
    void SetUIVolume(float volume)
    {
        Engine::AudioSystem::setBusVolume("bus:/UI", volume);
    }
    
    float GetMusicVolume()
    {
        float volume;
        Engine::AudioSystem::getBusVolume("bus:/Music", volume);
        return volume;
    }
};

// ==============================================================
// PAUSE GAME (Pause all audio)
// ==============================================================
class GamePauseHandler
{
public:
    void PauseGame()
    {
        // Pauses ALL active event instances
        Engine::AudioSystem::pauseAll(true);
    }
    
    void ResumeGame()
    {
        // Resumes ALL paused event instances
        Engine::AudioSystem::pauseAll(false);
    }
};

// ==============================================================
// COMPLETE GAME EXAMPLE
// ==============================================================
class GameAudioManager
{
private:
    MusicController music;
    AudioSettings settings;
    Math::Vec2D cameraPosition;

public:
    void Init()
    {
        // Load banks
        Engine::AudioSystem::loadBank("Assets/sounds/Master.bank");
        Engine::AudioSystem::loadBank("Assets/sounds/Master.strings.bank");
        Engine::AudioSystem::loadBank("Assets/sounds/Music.bank");
        Engine::AudioSystem::loadBank("Assets/sounds/SFX.bank");
        
        // Start menu music
        music.PlayMainMenu();
    }
    
    void OnGameStart()
    {
        music.StopMusic();
        music.PlayGameMusic();
    }
    
    void OnGamePause()
    {
        Engine::AudioSystem::pauseAll(true);
    }
    
    void OnGameResume()
    {
        Engine::AudioSystem::pauseAll(false);
    }
    
    void UpdateListener(const Math::Vec2D& camPos)
    {
        cameraPosition = camPos;
        Engine::AudioSystem::setListenerPosition(cameraPosition);
    }
    
    void OnPlayerJump(const Math::Vec2D& playerPos)
    {
        Engine::AudioSystem::playEventAtPosition("event:/Player/Jump", playerPos);
    }
    
    void OnButtonClick()
    {
        Engine::AudioSystem::playEvent("event:/UI/ButtonClick");
    }
    
    void ApplySettings(float masterVol, float musicVol, float sfxVol)
    {
        settings.SetMasterVolume(masterVol);
        settings.SetMusicVolume(musicVol);
        settings.SetSFXVolume(sfxVol);
    }
};

// ==============================================================
// HOW TO USE IN YOUR GAME
// ==============================================================
/*

// 1. In your Core::init() or equivalent:
Engine::AudioSystem::init();
Engine::AudioSystem::loadBank("Assets/sounds/Master.bank");
Engine::AudioSystem::loadBank("Assets/sounds/Master.strings.bank");

// 2. In your main game loop (already done in Core::run()):
Engine::AudioSystem::update();

// 3. Update listener position every frame:
Engine::AudioSystem::setListenerPosition(camera_position);

// 4. Play sounds when needed:
Engine::AudioSystem::playEvent("event:/UI/ButtonClick");

// 5. For looping/persistent sounds:
Engine::AudioSystem::createEventInstance("music", "event:/Music/Theme");
Engine::AudioSystem::startEventInstance("music");

// 6. On cleanup (already done in Core::exit()):
Engine::AudioSystem::shutdown();

*/
