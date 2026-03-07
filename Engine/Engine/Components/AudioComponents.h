//-----------------------------------------------------------------------------------
// file:    AudioComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for audio.
//          Includes Sound component for handling entity audio.
//          NEW: FMOD Studio integration for advanced audio features.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Audio/AudioSystem.h"
#include "../Reflection/ReflectionLibrary.h"
#include "../ECS/Macros.h"
#include <string>

namespace Components
{
  // ========================================================================
  // NEW: FMOD Studio Audio Components
  // ========================================================================

  // Component for playing FMOD Studio events (one-shot sounds)
  struct FMODEvent
  {
    ::std::string event_path;  // e.g., "event:/SFX/MonkeyHurt"

    FMODEvent() = default;
    FMODEvent(const ::std::string& path) : event_path(path) {}
  };

  // Component for persistent FMOD event instances (looping sounds, music)
  struct FMODEventInstance
  {
    ::std::string event_path;      // e.g., "event:/Music/Gameplay"
    ::std::string instance_name;   // Unique name for this instance
    bool auto_play = false;      // Start playing when created
    bool is_playing = false;

    FMODEventInstance() = default;
    FMODEventInstance(const ::std::string& path, const ::std::string& name, bool play = false)
      : event_path(path), instance_name(name), auto_play(play) {
    }
  };

  // State Audio for FMOD Studio (similar to your existing StateAudio pattern)
  struct FMODStateAudio
  {
    ::std::string event_path;      // FMOD event path
    bool loop = false;           // If true, creates persistent instance
    float volume = 1.0f;
    bool stop_on_state_exit = true;  // Stop when state exits

    FMODStateAudio() = default;
    FMODStateAudio(const ::std::string& path, bool should_loop = false, float vol = 1.0f)
      : event_path(path), loop(should_loop), volume(vol) {
    }
  };

  // ========================================================================
  // FMOD Helper Functions (use these to control FMOD components)
  // ========================================================================

  // Play an FMODEvent
  inline void PlayFMODEvent(const FMODEvent& event) {
    if (!event.event_path.empty()) {
      Engine::AudioSystem::playEvent(event.event_path);
    }
  }

  // Play an FMODEvent at position
  inline void PlayFMODEvent(const FMODEvent& event, const Math::Vec2D& position) {
    if (!event.event_path.empty()) {
      Engine::AudioSystem::playEventAtPosition(event.event_path, position);
    }
  }

  // Start an FMODEventInstance
  inline void StartFMODInstance(FMODEventInstance& instance) {
    if (!instance.event_path.empty() && !instance.instance_name.empty() && !instance.is_playing) {
      Engine::AudioSystem::createEventInstance(instance.instance_name, instance.event_path);
      Engine::AudioSystem::startEventInstance(instance.instance_name);
      instance.is_playing = true;
    }
  }

  // Stop an FMODEventInstance
  inline void StopFMODInstance(FMODEventInstance& instance, bool immediate = false) {
    if (instance.is_playing) {
      Engine::AudioSystem::stopEventInstance(instance.instance_name, immediate);
      Engine::AudioSystem::releaseEventInstance(instance.instance_name);
      instance.is_playing = false;
    }
  }

  // Set volume for FMODEventInstance
  inline void SetFMODInstanceVolume(const FMODEventInstance& instance, float volume) {
    if (instance.is_playing) {
      Engine::AudioSystem::setEventInstanceVolume(instance.instance_name, volume);
    }
  }
}

// Register components for reflection
REGISTER_COMPONENT(Components::FMODEvent, event_path)
REGISTER_COMPONENT(Components::FMODEventInstance, event_path, instance_name, auto_play, is_playing)
REGISTER_COMPONENT(Components::FMODStateAudio, event_path, loop, volume, stop_on_state_exit)
