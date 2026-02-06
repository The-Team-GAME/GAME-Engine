//-----------------------------------------------------------------------------------
// file:    AudioSystem.h
// project: Monkey Stole My GPA
//C
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
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Math/Vector2D.h"
#include "../TypeDef.h"
#include <memory>
#include <string>


namespace Engine {
  namespace AudioSystem {

    // ------------------------------------------------------------
    // Audio system lifecycle
    // ------------------------------------------------------------
    bool init();       // Initialize FMOD Studio
    void update();     // Must be called every frame
    void shutdown();   // Shutdown FMOD cleanly

    // ------------------------------------------------------------
    // Bank management
    // ------------------------------------------------------------
    bool loadBank(const std::string& path);
    bool unloadBank(const std::string& path);
    void unloadAllBanks();
    bool isBankLoaded(const std::string& path);

    // ------------------------------------------------------------
    // Event playback (one-shot)
    // ------------------------------------------------------------
    bool playEvent(const std::string& eventPath);
    bool playEventAtPosition(const std::string& eventPath, const Math::Vec2D& position);

    // ------------------------------------------------------------
    // Event instance management (persistent events)
    // ------------------------------------------------------------
    bool createEventInstance(const std::string& instanceName, const std::string& eventPath);
    bool startEventInstance(const std::string& instanceName);
    bool stopEventInstance(const std::string& instanceName, bool immediate = false);
    bool pauseEventInstance(const std::string& instanceName, bool pause);
    bool releaseEventInstance(const std::string& instanceName);
    void releaseAllEventInstances();
    bool isEventInstancePlaying(const std::string& instanceName);

    // ------------------------------------------------------------
    // Event parameters
    // ------------------------------------------------------------
    bool setEventParameter(const std::string& instanceName, const std::string& paramName, float value);
    bool getEventParameter(const std::string& instanceName, const std::string& paramName, float& outValue);

    // ------------------------------------------------------------
    // 3D audio positioning
    // ------------------------------------------------------------
    void setListenerPosition(const Math::Vec2D& position);
    void setListenerAttributes(const Math::Vec2D& position, const Math::Vec2D& velocity, const Math::Vec2D& forward, const Math::Vec2D& up);
    bool setEventInstancePosition(const std::string& instanceName, const Math::Vec2D& position);
    bool setEventInstanceAttributes(const std::string& instanceName, const Math::Vec2D& position, const Math::Vec2D& velocity);

    // ------------------------------------------------------------
    // Volume controls
    // ------------------------------------------------------------
    bool setEventInstanceVolume(const std::string& instanceName, float volume);
    bool setBusVolume(const std::string& busPath, float volume);
    bool setVCAVolume(const std::string& vcaPath, float volume);
    bool getBusVolume(const std::string& busPath, float& outVolume);
    bool getVCAVolume(const std::string& vcaPath, float& outVolume);

    // ------------------------------------------------------------
    // Global controls
    // ------------------------------------------------------------
    void pauseAll(bool pause);
    void setMasterVolume(float volume);
    float getMasterVolume();

    // ------------------------------------------------------------
    // Utility
    // ------------------------------------------------------------
    bool isInitialized();
    void flushCommandQueue();

  } // namespace AudioSystem
} // namespace Engine
