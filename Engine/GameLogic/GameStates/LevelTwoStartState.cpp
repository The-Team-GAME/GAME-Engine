//-----------------------------------------------------------------------------------
// file:    LevelTwoStartState.cpp
// project: Monkey Stole My GPA
//
// author: T
//
// brief:   Implementation of LevelTwoStartState.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "LevelTwoStartState.h"

#include "GameplayState.h"

namespace {
  static constexpr const char* LEVEL_TWO_PATH = "levels/LevelTwo.json";
}

void LevelTwoStartState::load()
{
}

void LevelTwoStartState::init()
{
}

void LevelTwoStartState::update()
{
  // Note: SceneManager::load_scene clears LevelManager in GAME_BUILD, so we must
  // request LevelTwo via GameplayState's pending-level mechanism.
  Log::info("[LevelTwoStartState] Requesting LevelTwo via pending-level: {}", LEVEL_TWO_PATH);
  Engine::LevelManager::set_pending_level_to_open(LEVEL_TWO_PATH);
  Engine::SceneManager::load_scene("GameplayState");
}

void LevelTwoStartState::draw()
{
  Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });
}

void LevelTwoStartState::free()
{
}

void LevelTwoStartState::unload()
{
}
