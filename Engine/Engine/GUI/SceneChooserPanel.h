//-----------------------------------------------------------------------------------
// file:    SceneChooserPanel.h
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

#include "Precompiled.h"

/*=================================================================================
Function Brief:
Creates and renders an ImGui panel with buttons for switching between different
game states. Used during development for quick testing and navigation between
various engine features and demonstrations.

Where to call it:
Call this function once per frame during the ImGui rendering phase, typically
in the Draw() or Update() function of a game state.
==================================================================================*/
inline void SceneChooserPanel()
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
      // Use the style's default button color and brighten it slightly so the
      // 'current' scene stands out without hardcoding a specific color.
      auto brighten = [](ImVec4 c, float factor) -> ImVec4 {
        ImVec4 out = c;
        out.x = out.x * factor; if (out.x > 1.0f) out.x = 1.0f;
        out.y = out.y * factor; if (out.y > 1.0f) out.y = 1.0f;
        out.z = out.z * factor; if (out.z > 1.0f) out.z = 1.0f;
        // leave alpha as-is
        return out;
        };

      ImVec4 base = ImGui::GetStyle().Colors[ImGuiCol_Button];
      ImGui::PushStyleColor(ImGuiCol_Button, brighten(base, 1.50f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, brighten(base, 1.50f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, brighten(base, 1.50f));
    }

    // Create button for each scene
    if (ImGui::Button(scene_name.c_str())) {
      Engine::SceneManager::load_scene(scene_name);
    }

    if (is_current) {
      ImGui::PopStyleColor(3);
    }
  }

  // Show helpful info
  ImGui::Separator();

  ImGui::End();
}