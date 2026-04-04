//-----------------------------------------------------------------------------------
// file:    GameAudio.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   Simple helper functions for playing game sounds using FMOD Studio.
//          Just include this file and call the functions anywhere in your game!
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "AudioSystem.h"
#include "../Math/Vector2D.h"

namespace Engine {
  namespace GameAudio {

    // ============================================================
    // PLAYER SOUNDS
    // ============================================================
    
    // Play monkey hurt sound (simple - no position needed)
    inline void PlayMonkeyHurt() {
      AudioSystem::playEvent("event:/SFX/MonkeyHurt");
    }
    
    // Play monkey hurt sound at a specific position (for 3D audio)
    inline void PlayMonkeyHurt(const Math::Vec2D& position) {
      AudioSystem::playEventAtPosition("event:/SFX/MonkeyHurt", position);
    }
    
    // Play monkey jump sound
    inline void PlayMonkeyJump() {
      AudioSystem::playEvent("event:/SFX/MonkeyJump");
    }
    
    inline void PlayMonkeyJump(const Math::Vec2D& position) {
      AudioSystem::playEventAtPosition("event:/SFX/MonkeyJump", position);
    }
    
    // Play monkey death sound
    inline void PlayMonkeyDeath() {
      AudioSystem::playEvent("event:/SFX/MonkeyDeath");
    }
    
    // Play footstep sound
    inline void PlayFootstep(const Math::Vec2D& position) {
      AudioSystem::playEventAtPosition("event:/SFX/Footstep", position);
    }

    // ============================================================
    // UI SOUNDS
    // ============================================================
    
    inline void PlayButtonClick() {
      AudioSystem::playEvent("event:/SFX/ButtonClick");
    }
    
    inline void PlayButtonHover() {
      AudioSystem::playEvent("event:/SFX/ButtonHover");
    }
    
    inline void PlayMenuOpen() {
      AudioSystem::playEvent("event:/SFX/MenuOpen");
    }
    
    inline void PlayMenuClose() {
      AudioSystem::playEvent("event:/SFX/MenuClose");
    }

    // ============================================================
    // GAME SOUNDS
    // ============================================================
    
    inline void PlayCollectItem(const Math::Vec2D& position) {
      AudioSystem::playEventAtPosition("event:/SFX/CollectItem", position);
    }
    
    inline void PlayExplosion(const Math::Vec2D& position) {
      AudioSystem::playEventAtPosition("event:/SFX/Explosion", position);
    }

    // ============================================================
    // BACKGROUND MUSIC
    // ============================================================
    
    inline void PlayMainMenuMusic() {
      // Stop any existing music first
      if (AudioSystem::isEventInstancePlaying("music")) {
        AudioSystem::stopEventInstance("music");
        AudioSystem::releaseEventInstance("music");
      }
      
      AudioSystem::createEventInstance("music", "event:/Music/MainMenu");
      AudioSystem::startEventInstance("music");
    }
    
    inline void PlayGameplayMusic() {
      if (AudioSystem::isEventInstancePlaying("music")) {
        AudioSystem::stopEventInstance("music");
        AudioSystem::releaseEventInstance("music");
      }
      
      AudioSystem::createEventInstance("music", "event:/Music/Gameplay");
      AudioSystem::startEventInstance("music");
    }
    
    inline void StopMusic() {
      if (AudioSystem::isEventInstancePlaying("music")) {
        AudioSystem::stopEventInstance("music");
        AudioSystem::releaseEventInstance("music");
      }
    }
    
    inline void PauseMusic() {
      AudioSystem::pauseEventInstance("music", true);
    }
    
    inline void ResumeMusic() {
      AudioSystem::pauseEventInstance("music", false);
    }

    // ============================================================
    // VOLUME CONTROLS
    // ============================================================
    
    inline void SetMusicVolume(float volume) {
      AudioSystem::setBusVolume("bus:/Music", volume);
    }
    
    inline void SetSFXVolume(float volume) {
      AudioSystem::setBusVolume("bus:/SFX", volume);
    }
    
    inline void SetMasterVolume(float volume) {
      AudioSystem::setMasterVolume(volume);
    }

  } // namespace GameAudio
} // namespace Engine


