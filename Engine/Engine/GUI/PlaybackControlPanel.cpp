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

  // Display current state with color coding
  ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Playback State");
  ImGui::Separator();

  if (is_paused) {
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "PAUSED");
  }
  else {
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "PLAYING");
  }

  ImGui::Separator();
  ImGui::Spacing();

  // Button row
  ImGui::BeginGroup();

  // Play button (green when playing)
  if (is_paused) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.8f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.9f, 0.2f, 1.0f));
  }
  else {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.7f, 0.1f, 1.0f));
  }

  if (ImGui::Button("Play", ImVec2(80, 40))) {
    Engine::Core::set_paused(false);
  }
  ImGui::PopStyleColor(3);

  ImGui::SameLine();

  // Pause button (yellow when paused)
  if (!is_paused) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.1f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.7f, 0.1f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.8f, 0.1f, 1.0f));
  }
  else {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.4f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.5f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.6f, 0.0f, 1.0f));
  }

  if (ImGui::Button("Pause", ImVec2(80, 40))) {
    Engine::Core::set_paused(true);
  }
  ImGui::PopStyleColor(3);

  ImGui::SameLine();

  // Stop button (red)
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 0.6f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));

  if (ImGui::Button("Stop", ImVec2(80, 40))) {
    // Stop = reload current scene + pause after the next frame completes
    std::string current = Engine::SceneManager::get_current_scene_name();
    Engine::SceneManager::unload_current_scene();
    Engine::SceneManager::load_scene(current);

    // Defer pause to next frame so the scene reload can complete and render
    Engine::Core::set_paused(true, true);
  }
  ImGui::PopStyleColor(3);

  ImGui::EndGroup();

  ImGui::Spacing();
  ImGui::Separator();


  // Keyboard shortcuts info
  ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Keyboard Shortcuts:");
  ImGui::Text("P - Toggle Play/Pause");
  ImGui::Text("] - Step forward (when paused)");

  ImGui::Spacing();
  ImGui::Separator();

  // Additional info
  ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Note:");
  ImGui::TextWrapped("When paused, physics and game logic updates are halted. Use the step forward key (]) to advance one frame at a time.");

  ImGui::End();
}
