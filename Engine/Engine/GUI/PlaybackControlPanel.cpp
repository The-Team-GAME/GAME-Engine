//-----------------------------------------------------------------------------------
// file:    PlaybackControlPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of PlaybackControlPanel for controlling game execution
//          with Play, Pause, and Stop buttons. Provides visual feedback and
//          integrates with Core's physics_step_mode.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "PlaybackControlPanel.h"
#include "../Core.h"
#include "../SceneManager.h"

void PlaybackControlPanel() {
  ImGui::Begin("Playback Control");

  // Get current state
  bool is_paused = Engine::Core::is_paused();

  // Button row
  ImGui::BeginGroup();

  // Play button - highlight when playing
  if (!is_paused) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.75f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.55f, 0.1f, 1.0f));
  }
  else {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.55f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.45f, 0.1f, 1.0f));
  }

  if (ImGui::Button("Play")) {
    Engine::Core::set_paused(false);
  }
  ImGui::PopStyleColor(3);

  ImGui::SameLine();

  // Pause button - highlight when paused
  if (is_paused) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.6f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.5f, 0.1f, 1.0f));
  }
  else {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.5f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.4f, 0.08f, 1.0f));
  }

  if (ImGui::Button("Pause")) {
    Engine::Core::set_paused(true);
  }
  ImGui::PopStyleColor(3);

  ImGui::SameLine();

  // Stop button
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.2f, 0.2f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.15f, 0.15f, 1.0f));

  if (ImGui::Button("Stop")) {
    // Stop = reload current scene + pause after the next frame completes
    std::string current = Engine::SceneManager::get_current_scene_name();
    Engine::SceneManager::unload_current_scene();
    Engine::SceneManager::load_scene(current);

    // Defer pause to next frame so the scene reload can complete and render
    Engine::Core::set_paused(true, true);
  }
  ImGui::PopStyleColor(3);

  ImGui::EndGroup();

  ImGui::End();
}
