//-----------------------------------------------------------------------------------
// file:    LevelPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the LevelPanel ImGui interface for managing game levels
//          including saving, loading, and component-based filtering.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "LevelPanel.h"
#include "../Level.h"
#include "../FileIO/LevelFileIO.h"
#include "../Path.h"
#include "../Logger.h"
#include <unordered_map>
#include "../Resources.h"

void LevelPanel() {
  ImGui::Begin("Level Manager");

  static char level_name[128] = "NewLevel";
  static char level_path[256] = "levels/level.json";


  // Level info section
  if (ImGui::CollapsingHeader("Level Info", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::InputText("Level Name", level_name, sizeof(level_name));
    ImGui::InputText("File Path", level_path, sizeof(level_path));
    ImGui::TextWrapped("Path is relative to Assets/ folder");
  }

  ImGui::Separator();

  // Save section
  ImGui::Text("Entities to save: %zu (all)", Engine::ECS::get_entity_count());

  ImGui::Separator();

  if (ImGui::Button("Save Level", ImVec2(-1, 0))) {
    std::string full_path = Engine::Path("Assets").join(level_path).to_string();

    // Save all entities
    Engine::Level level = Engine::Level::from_world();
    Engine::Resources::save_level(full_path, level);
  }

  ImGui::Separator();

  // Load section
  static bool clear_existing = true;
  ImGui::Checkbox("Clear existing entities before loading", &clear_existing);

  ImGui::Separator();

  if (ImGui::Button("Load Level", ImVec2(-1, 0))) {
    std::string full_path = Engine::Path("Assets").join(level_path).to_string();
    auto level = Engine::Resources::load_level(full_path, clear_existing);
    level.instantiate_into_world();
  }

  ImGui::End();
}
