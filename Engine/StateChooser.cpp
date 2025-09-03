#include "imgui.h."
#include "StateManager.h"
#include "BasicState.h"
#include "MainMenuState.h"

void StateChooser()
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

  ImGui::End();
}