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

  std::string current_scene = Engine::SceneManager::get_current_scene_name();
  ImGui::Text("Current State: %s", current_scene.c_str());


  ImGui::Separator();
  ImGui::Text("Available Scenes (%d):", (int)Engine::SceneManager::get_all_scene_names().size());

  // Get all registered scenes dynamically
  auto scene_names = Engine::SceneManager::get_all_scene_names();

  // Loop through all scenes and create buttons
  for (const auto& scene_name : scene_names) {
    // Highlight current scene button
    bool is_current = (scene_name == current_scene);

    if (is_current) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 0.0f, 1.0f)); // Green
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
    }

    // Create button for each scene
    if (ImGui::Button(scene_name.c_str(), ImVec2(200, 0))) {
      if (!is_current) { // Only switch if it's not the current scene
        Engine::SceneManager::load_scene(scene_name);
      }
    }

    if (is_current) {
      ImGui::PopStyleColor(3);
      ImGui::SameLine();
      ImGui::Text(" = Current");
    }
  }

  // Show helpful info
  ImGui::Separator();
  ImGui::TextDisabled("Scenes are loaded from SceneManager registry");

  ImGui::End();
}