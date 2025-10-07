//-----------------------------------------------------------------------------------
// file:    StateChooser.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines an ImGui panel that provides buttons for switching
//          between different game states during development and testing.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "imgui.h"
#include "StateManager.h"
#include "BasicState.h"
#include "PhysicsTestState.h"
#include "SpriteState.h"
#include "GameObjectState.h"
#include "DevManySpritesState.h"
#include "Rubric2500ParticlesState.h"
#include "RectPainterState.h"
#include "DevSpriteAnimationState.h"
#include "DevECSMessageState.h"
#include "LevelLoaderTestState.h"
#include "ShowcaseState.h"

/*=================================================================================
Function Brief:
Creates and renders an ImGui panel with buttons for switching between different
game states. Used during development for quick testing and navigation between
various engine features and demonstrations.

Where to call it:
Call this function once per frame during the ImGui rendering phase, typically
in the Draw() or Update() function of a game state.
==================================================================================*/
inline void StateChooserPanel()
{
  ImGui::Begin("State Chooser");

  ImGui::Text("Current State: %s", typeid(*Engine::StateManager::get_current_state()).name());


  if (ImGui::Button("ShowcaseState"))
  {
    Engine::StateManager::change_state(ShowcaseState::get());
  }
  if (ImGui::Button("RectPainterState"))
  {
    Engine::StateManager::change_state(RectPainterState::get());
  }
  if (ImGui::Button("Rubric2500ParticlesState"))
  {
    Engine::StateManager::change_state(Rubric2500ParticlesState::get());
  }
  if (ImGui::Button("DevManySpritesState"))
  {
    Engine::StateManager::change_state(DevManySpritesState::get());
  }
  if (ImGui::Button("PhysicsTestState"))
  {
    Engine::StateManager::change_state(PhysicsTestState::get());
  }
  if (ImGui::Button("LevelLoaderTestState"))
  {
    Engine::StateManager::change_state(LevelLoaderTestState::get());
  }

  // Other states for development and testing - uncomment as needed
  // 
  // if (ImGui::Button("BasicState"))
  // {
  //   Engine::StateManager::change_state(BasicState::Get());
  // }
  // if (ImGui::Button("SpriteState"))
  // {
  //   Engine::StateManager::change_state(SpriteState::Get());
  // }
  // if (ImGui::Button("GameObjectState"))
  // {
  //   Engine::StateManager::change_state(GameObjectState::Get());
  // }
  // if (ImGui::Button("DevSpriteAnimationState"))
  // {
  //   Engine::StateManager::change_state(DevSpriteAnimationState::Get());
  // }
  // if (ImGui::Button("DevECSMessageState"))
  // {
  //   Engine::StateManager::change_state(DevECSMessageState::Get());
  // }


  ImGui::End();
}