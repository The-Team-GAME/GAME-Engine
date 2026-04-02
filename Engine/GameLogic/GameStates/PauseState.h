//-----------------------------------------------------------------------------------
// file:    PauseState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:    
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"
#include "GameplayOverlayInputGuard.h"

namespace PauseStateDetail {
  void start_pause_lowpass_snapshot();
  void stop_pause_lowpass_snapshot();
}

class PauseState : public Engine::Scene
{
  static constexpr const char* PAUSE_OVERLAY_LEVEL = "levels/NewPause.json";
  static constexpr const char* PAUSE_OPEN_EVENT = "event:/SFXUI/ButtonPause";
  static constexpr const char* PAUSE_CLOSE_EVENT = "event:/SFXUI/ButtonClick";

  std::string overlay_level_id_;

public:
  void load() {
    GameplayOverlayInputGuard::acquire();
    PauseStateDetail::start_pause_lowpass_snapshot();
    Engine::AudioSystem::playEvent(PAUSE_OPEN_EVENT);
    overlay_level_id_ = Engine::LevelManager::open_overlay_level(PAUSE_OVERLAY_LEVEL);
    Engine::Time::set_time_scale(0.0001f);
  };

  void init() {};

  void update() {
    if (overlay_level_id_.empty() && Engine::Input::is_pressed(Engine::KeyCode::Escape)) {
      Engine::SceneManager::pop_scene();
    }
  };

  void draw() {};

  void free() {};

  void unload() {
    PauseStateDetail::stop_pause_lowpass_snapshot();
    Engine::AudioSystem::playEvent(PAUSE_CLOSE_EVENT);

    if (!overlay_level_id_.empty()) {
      Engine::LevelManager::close_overlay_level(overlay_level_id_);
      overlay_level_id_.clear();
    }

    Engine::Time::set_time_scale(1.0f);
    GameplayOverlayInputGuard::release();
  };
};