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
#include "../../Engine/MonkeyEngine.h"
#include "../GameStates/EmptyState.h"
#include "../GameStates/MainMenuState.h"
#include "../GameStates/BasicState.h"
#include "../GameStates/Rubric2500ParticlesState.h"
#include "../GameStates/RectPainterState.h"
#include "../GameStates/DevSpriteAnimationState.h"
#include "../GameStates/ShowcaseState.h"
#include "../GameStates/Documentation/PrefabsState.h"
#include "../GameStates/Dev/PhysicsTestState.h"
#include "../GameStates/Documentation/LayersState.h"

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

  if (ImGui::Button("EmptyState"))
  {
    Engine::StateManager::change_state(EmptyState::get());
  }
  if (ImGui::Button("MainMenuState"))
  {
    Engine::StateManager::change_state(MainMenuState::get());
  }
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
  if (ImGui::Button("BasicState"))
  {
    Engine::StateManager::change_state(BasicState::get());
  }
  if (ImGui::Button("DevSpriteAnimationState"))
  {
    Engine::StateManager::change_state(DevSpriteAnimationState::get());
  }
  if (ImGui::Button("PrefabsState"))
  {
    Engine::StateManager::change_state(PrefabsState::get());
  }
  if (ImGui::Button("PhysicsTestState"))
  {
    Engine::StateManager::change_state(PhysicsTestState::get());
  }
  if (ImGui::Button("LayersState"))
  {
    Engine::StateManager::change_state(LayersState::get());
  }

  ImGui::End();
}