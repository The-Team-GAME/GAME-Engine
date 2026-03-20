//-----------------------------------------------------------------------------------
// file:    MainMenuState.h
// project: Monkey Stole My GPA
//
// brief:   Main menu scene that loads MainMenuStart.json as its level.
//          All visuals, animations, and state transitions (Start -> Menu) are
//          driven by the level JSON and main_menu.lua. This class only handles
//          audio lifecycle and level loading.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class MainMenuState : public Engine::Scene
{
  static constexpr const char* MAIN_MENU_BGM_EVENT = "event:/BGM/MainMenu";
  static constexpr const char* SPLASH_SFX_SNORE_EVENT = "event:/SFX/SplashSnoreSFX";
  static constexpr const char* SPLASH_SFX_TRAIN_EVENT = "event:/SFX/SplashTrainSFX";
  static constexpr const char* MAIN_MENU_HEARTBEAT_EVENT = "event:/SFX/PlayerHeartBeat";

public:
  void load() {
    Engine::LevelManager::open_level("levels/MainMenuStart.json");

    Engine::AudioSystem::stopEvent(MAIN_MENU_HEARTBEAT_EVENT, true);
    Engine::AudioSystem::stopSceneMusic(true);
    Engine::AudioSystem::stopEvent(SPLASH_SFX_SNORE_EVENT, true);
    Engine::AudioSystem::playEvent(SPLASH_SFX_SNORE_EVENT);
    Engine::AudioSystem::stopEvent(SPLASH_SFX_TRAIN_EVENT, true);
    Engine::AudioSystem::playEvent(SPLASH_SFX_TRAIN_EVENT);
  };

  void init() {};

  void update() {};

  void draw() {};

  void free() {};

  void unload() {
    Engine::AudioSystem::stopEvent(SPLASH_SFX_SNORE_EVENT, true);
    Engine::AudioSystem::stopEvent(SPLASH_SFX_TRAIN_EVENT, true);
    Engine::AudioSystem::stopSceneMusic(true);
  };
};
