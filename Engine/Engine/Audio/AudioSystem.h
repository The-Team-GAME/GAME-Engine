//-----------------------------------------------------------------------------------
// file:    AudioSystem.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   Public interface for the FMOD-based audio system. Provides sound
//          loading, playback control, and spatial audio functionality.
//          Supports 2D and 3D audio positioning with volume and looping controls.
//          Integrates with the resource management system for automatic cleanup.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Math/Vector2D.h"
#include "../TypeDef.h"
#include "../EngineExport.h"
#include <memory>
#include <string>
#include <vector>


namespace Engine {
  namespace AudioSystem {

    // ------------------------------------------------------------
    // Audio system lifecycle
    // ------------------------------------------------------------
    ENGINE_API bool init();       // Initialize FMOD Studio
    ENGINE_API void update();     // Must be called every frame
    ENGINE_API void shutdown();   // Shutdown FMOD cleanly

    // ------------------------------------------------------------
    // Bank management
    // ------------------------------------------------------------
    ENGINE_API bool loadBank(const std::string& path);
    ENGINE_API bool unloadBank(const std::string& path);
    ENGINE_API void unloadAllBanks();
    ENGINE_API bool isBankLoaded(const std::string& path);

    // ------------------------------------------------------------
    // Event playback (one-shot)
    // ------------------------------------------------------------
    ENGINE_API bool playEvent(const std::string& eventPath);
    ENGINE_API bool stopEvent(const std::string& eventPath, bool immediate = true);
    // minDistance: radius where sound is full volume and mostly centered (default 500)
    // maxDistance: radius where sound fades to silence (default 2000)
    // soundSpread: 0-100, how much sound spreads between speakers when close (0=point source, 100=wide stereo)
    ENGINE_API bool playEventAtPosition(const std::string& eventPath, const Math::Vec2D& position, float minDistance = 500.0f, float maxDistance = 2000.0f, float soundSpread = 50.0f, float volume = 1.0f);

    // ------------------------------------------------------------
    // Event instance management (persistent events)
    // ------------------------------------------------------------
    ENGINE_API bool createEventInstance(const std::string& instanceName, const std::string& eventPath);
    ENGINE_API bool startEventInstance(const std::string& instanceName);
    ENGINE_API bool stopEventInstance(const std::string& instanceName, bool immediate = false);
    ENGINE_API bool pauseEventInstance(const std::string& instanceName, bool pause);
    ENGINE_API bool releaseEventInstance(const std::string& instanceName);
    ENGINE_API void releaseAllEventInstances();
    ENGINE_API bool isEventInstancePlaying(const std::string& instanceName);

    // ------------------------------------------------------------
    // Event parameters
    // ------------------------------------------------------------
    ENGINE_API bool setEventParameter(const std::string& instanceName, const std::string& paramName, float value);
    ENGINE_API bool getEventParameter(const std::string& instanceName, const std::string& paramName, float& outValue);

    // ------------------------------------------------------------
    // 3D audio positioning
    // ------------------------------------------------------------
    ENGINE_API void setListenerPosition(const Math::Vec2D& position, float depth = 0.0f);
    ENGINE_API void setListenerAttributes(const Math::Vec2D& position, const Math::Vec2D& velocity, const Math::Vec2D& forward, const Math::Vec2D& up);
    ENGINE_API bool setEventInstancePosition(const std::string& instanceName, const Math::Vec2D& position);
    ENGINE_API bool setEventInstanceAttributes(const std::string& instanceName, const Math::Vec2D& position, const Math::Vec2D& velocity);

    // ------------------------------------------------------------
    // Volume controls
    // ------------------------------------------------------------
    ENGINE_API bool setEventInstanceVolume(const std::string& instanceName, float volume);
    ENGINE_API bool setBusVolume(const std::string& busPath, float volume);
    ENGINE_API bool setVCAVolume(const std::string& vcaPath, float volume);
    ENGINE_API bool getBusVolume(const std::string& busPath, float& outVolume);
    ENGINE_API bool getVCAVolume(const std::string& vcaPath, float& outVolume);

    // Scene BGM helpers (single managed BGM instance across scene transitions)
    ENGINE_API bool playSceneMusic(const std::string& eventPath, float volume = 1.0f, bool immediateStop = true);
    ENGINE_API bool stopSceneMusic(bool immediate = true);
    ENGINE_API bool isSceneMusicPlaying();

    // ------------------------------------------------------------
    // Global controls
    // ------------------------------------------------------------
    ENGINE_API void pauseAll(bool pause);
    ENGINE_API void stopAll(bool immediate = false);
    ENGINE_API void setMasterVolume(float volume);
    ENGINE_API float getMasterVolume();

    // Bus-based volume controls (maps to FMOD Studio buses)
    ENGINE_API void setBGMVolume(float volume);
    ENGINE_API void setSFXVolume(float volume);
    ENGINE_API float getBGMVolume();
    ENGINE_API float getSFXVolume();

    // ------------------------------------------------------------
    // Event enumeration (for editor dropdowns)
    // ------------------------------------------------------------
    ENGINE_API std::vector<std::string> getAllEventPaths();
    ENGINE_API std::vector<std::string> getAllBusPaths();

    // ------------------------------------------------------------
    // FMOD Core access (for subsystems that need raw PCM playback)
    // ------------------------------------------------------------
    ENGINE_API void* getCoreSystem();

    // ------------------------------------------------------------
    // Utility
    // ------------------------------------------------------------
    ENGINE_API bool isInitialized();
    ENGINE_API void flushCommandQueue();

  } // namespace AudioSystem
} // namespace Engine
