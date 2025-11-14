//-----------------------------------------------------------------------------------
// file:    LevelManagerPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the LevelManagerPanel ImGui interface for managing
//          game levels. Provides functionality to create, load, save, and switch
//          between levels at runtime, with integration into the ProjectSaveManager
//          for dirty state tracking.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "LevelManagerPanel.h"
#include "../Level.h"
#include "../Resources.h"
#include "../ProjectSaveManager.h"
#include "../Logger.h"
#include "../Path.h"
#include "../FileIO/LevelFileIO.h"
#include <vector>
#include <algorithm>

void LevelManagerPanel()
{
  static std::string selected_level_path = "";
  static char levelSearchBuffer[256] = "";

  ImGui::Begin("Level Manager");

  // Section 1: Current Level Info
  if (ImGui::CollapsingHeader("Current Level", ImGuiTreeNodeFlags_DefaultOpen))
  {
    std::string current_level_name = Engine::LevelManager::get_current_level_name();

    if (current_level_name.empty()) {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No level currently loaded");
    }
    else {
      ImGui::Text("Active Level: %s", current_level_name.c_str());

      ImGui::Separator();

      // Save current level button
      if (ImGui::Button("Save Current Level")) {
        Engine::Level current_level = Engine::Level::from_world(current_level_name);
        std::string level_path = "levels/" + current_level_name;

        if (Engine::Resources::save_level(level_path, current_level, true)) {
          Log::info("Saved current level: " + current_level_name);
        }
        else {
          Log::error("Failed to save current level: " + current_level_name);
        }
      }

      ImGui::SameLine();

      // Mark current level as dirty
      if (ImGui::Button("Mark as Dirty")) {
        Engine::ProjectSaveManager::set_dirty(
          Engine::ProjectSaveManager::DirtyStates::Level(current_level_name)
        );
        Log::info("Marked level as dirty: " + current_level_name);
      }
    }
  }

  ImGui::Separator();

  // Section 2: Level List
  if (ImGui::CollapsingHeader("Levels", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // Search/Filter
    ImGui::InputText("Search", levelSearchBuffer, 256);

    ImGui::Separator();

    // List available level files
    ImGui::BeginChild("LevelList", ImVec2(0, 200), true);

    // Get levels from cache
    std::vector<std::pair<std::string, Engine::Level>> level_list;

    for (const auto& [path, level] : Engine::Resources::level_cache) {
      // Get relative path for display
      std::string relative_path = Engine::Resources::get_relative_path(path);

      // Apply search filter
      if (strlen(levelSearchBuffer) > 0 && relative_path.find(levelSearchBuffer) == std::string::npos) {
        continue;
      }

      level_list.push_back({ relative_path, level });
    }

    // Sort by path for consistent ordering
    std::sort(level_list.begin(), level_list.end(),
      [](const auto& a, const auto& b) { return a.first < b.first; });

    for (const auto& [relative_path, level] : level_list) {
      bool isSelected = (selected_level_path == relative_path);

      std::string display_label = relative_path;
      if (!level.name.empty()) {
        display_label += " (" + level.name + ")";
      }
      display_label += " [" + std::to_string(level.entities.size()) + " entities]";

      if (ImGui::Selectable(display_label.c_str(), isSelected)) {
        selected_level_path = relative_path;
      }
    }

    ImGui::EndChild();

    // Create new level button
    static char newLevelNameBuffer[256] = "";
    if (ImGui::Button("Create New Level")) {
      ImGui::OpenPopup("Create New Level");
      // Clear buffer when opening popup
      newLevelNameBuffer[0] = '\0';
    }

    // Create New Level popup
    if (ImGui::BeginPopup("Create New Level"))
    {
      ImGui::Text("Enter level name:");
      ImGui::InputText("##NewLevelName", newLevelNameBuffer, sizeof(newLevelNameBuffer));

      ImGui::Separator();

      if (ImGui::Button("Create")) {
        std::string level_name = std::string(newLevelNameBuffer);

        if (!level_name.empty()) {
          // Add .json extension if not present
          if (level_name.find(".json") == std::string::npos) {
            level_name += ".json";
          }

          // Construct the relative path (levels folder)
          std::string relative_path = "levels/" + level_name;

          // Create a new empty level from current world
          Engine::Level new_level = Engine::Level::from_world(level_name);

          // Add to cache without saving to disk
          std::string full_path = Engine::Path(Engine::Resources::resource_path).join(relative_path);
          Engine::Resources::level_cache[full_path] = new_level;

          // Mark as dirty in ProjectSaveManager
          Engine::ProjectSaveManager::set_dirty(
            Engine::ProjectSaveManager::DirtyStates::Level(level_name)
          );

          // Select the newly created level
          selected_level_path = relative_path;

          Log::info("Created new level: " + relative_path);
          ImGui::CloseCurrentPopup();
        }
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    ImGui::SameLine();

    // Load selected level button
    if (!selected_level_path.empty() && ImGui::Button("Load Selected")) {
      // Construct full path
      std::string full_path = Engine::Path(Engine::Resources::resource_path).join(selected_level_path);

      auto it = Engine::Resources::level_cache.find(full_path);
      if (it != Engine::Resources::level_cache.end()) {
        it->second.instantiate_into_world();
        Log::info("Loaded level: " + selected_level_path);
      }
      else {
        Log::error("Failed to load level - not found in cache: " + selected_level_path);
      }
    }

    ImGui::SameLine();

    // Save selected level button
    if (!selected_level_path.empty() && ImGui::Button("Save Selected")) {
      std::string full_path = Engine::Path(Engine::Resources::resource_path).join(selected_level_path);

      auto it = Engine::Resources::level_cache.find(full_path);
      if (it != Engine::Resources::level_cache.end()) {
        if (Engine::Resources::save_level(selected_level_path, it->second, true)) {
          Log::info("Saved level: " + selected_level_path);
        }
        else {
          Log::error("Failed to save level: " + selected_level_path);
        }
      }
      else {
        Log::error("Failed to save level - not found in cache: " + selected_level_path);
      }
    }
  }

  ImGui::Separator();

  // Section 3: Level Editor for Selected Level
  if (!selected_level_path.empty())
  {
    ImGui::Text("Editing: %s", selected_level_path.c_str());

    // Get the level from cache
    std::string full_path = Engine::Path(Engine::Resources::resource_path).join(selected_level_path);
    auto it = Engine::Resources::level_cache.find(full_path);

    if (it != Engine::Resources::level_cache.end()) {
      const Engine::Level& level = it->second;

      ImGui::Separator();

      ImGui::Text("Level Name: %s", level.name.c_str());
      ImGui::Text("Entity Count: %zu", level.entities.size());

      ImGui::Separator();

      // Update level from world button
      if (ImGui::Button("Update from Current World")) {
        // Create level from current world state
        Engine::Level updated_level = Engine::Level::from_world(level.name);

        // Update in cache
        Engine::Resources::level_cache[full_path] = updated_level;

        // Mark as dirty
        Engine::ProjectSaveManager::set_dirty(
          Engine::ProjectSaveManager::DirtyStates::Level(level.name)
        );

        Log::info("Updated level from world: " + level.name);
      }

      ImGui::SameLine();

      // Export to JSON
      if (ImGui::Button("Export to JSON (Clipboard)"))
      {
        std::string json = Engine::FileIO::serialize_level_to_json(level);
        ImGui::SetClipboardText(json.c_str());
        ImGui::OpenPopup("Export Success");
      }

      // Export success popup
      if (ImGui::BeginPopup("Export Success"))
      {
        ImGui::Text("Level JSON copied to clipboard!");
        ImGui::EndPopup();
      }
    }
    else {
      ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Level not found in cache!");
    }
  }
  else
  {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No level selected");
  }

  ImGui::End();
}
