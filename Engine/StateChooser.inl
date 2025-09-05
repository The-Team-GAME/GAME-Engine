#pragma once

#include "imgui.h."
#include "StateManager.h"
#include "BasicState.h"
#include "MainMenuState.h"
#include "EcsState.h"
#include "JsonState.h"
#include "SpriteState.h"

inline void StateChooser()
{
  ImGui::Begin("State Chooser");

  ImGui::Text("Current State: %s", typeid(*Engine::StateManager::GetCurrentState()).name());

  if (ImGui::Button("BasicState"))
  {
    Engine::StateManager::ChangeState(BasicState::Get());
  }
  if (ImGui::Button("MainMenuState"))
  {
    Engine::StateManager::ChangeState(MainMenuState::Get());
  }
  if (ImGui::Button("EcsState"))
  {
    Engine::StateManager::ChangeState(EcsState::Get());
  }
  if (ImGui::Button("JsonState"))
  {
    Engine::StateManager::ChangeState(JsonState::Get());
  }
  if (ImGui::Button("SpriteState"))
  {
    Engine::StateManager::ChangeState(SpriteState::Get());
  }

  ImGui::End();
}