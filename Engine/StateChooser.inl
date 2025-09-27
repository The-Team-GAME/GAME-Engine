#pragma once

#include "imgui.h"
#include "StateManager.h"
#include "BasicState.h"
#include "MainMenuState.h"
#include "EcsState.h"
#include "SpriteState.h"
#include "GameObjectState.h"
#include "DevManySpritesState.h"
#include "Rubric2500Particles.h"
#include "ImGuiState.h"
#include "DevSpriteAnimationState.h"
#include "DevECSMessageState.h"

inline void StateChooserPanel()
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
  if (ImGui::Button("SpriteState"))
  {
    Engine::StateManager::ChangeState(SpriteState::Get());
  }
  if (ImGui::Button("GameObjectState"))
  {
    Engine::StateManager::ChangeState(GameObjectState::Get());
  }
  if (ImGui::Button("DevManySpritesState"))
  {
    Engine::StateManager::ChangeState(DevManySpritesState::Get());
  }
  if (ImGui::Button("Rubric2500Particles"))
  {
    Engine::StateManager::ChangeState(Rubric2500Particles::Get());
  }
  if (ImGui::Button("ImGui Rect Painter"))
  {
    Engine::StateManager::ChangeState(ImGuiState::Get());
  }
  if(ImGui::Button("DevSpriteAnimationState"))
  {
     Engine::StateManager::ChangeState(DevSpriteAnimationState::Get());
  }
  if (ImGui::Button("DevECSMessageState"))
  {
    Engine::StateManager::ChangeState(DevECSMessageState::Get());
  }

  ImGui::End();
}