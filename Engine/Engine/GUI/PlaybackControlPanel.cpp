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

namespace Engine {
  namespace PlaybackControl {
    bool is_paused() {
      return Core::is_paused();
    }

    void set_paused(bool paused) {
      Core::set_paused(paused);
    }
  }
}

void PlaybackControlPanel() {
  ImGui::Begin("Playback Control");

  // Get current state
  bool is_paused = Engine::PlaybackControl::is_paused();

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
    Engine::PlaybackControl::set_paused(false);
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
    Engine::PlaybackControl::set_paused(true);
  }
  ImGui::PopStyleColor(3);

  ImGui::SameLine();

  // Stop button (red)
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 0.6f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));

  if (ImGui::Button("Stop", ImVec2(80, 40))) {
    // Stop = Pause + potentially reset to initial state
    // For now, we'll just pause. You can add reset logic here if needed
    Engine::PlaybackControl::set_paused(true);
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
