#include "ProjectSaveManager.h"
#include "Logger.h"
#include "FileIO/Settings.h"
#include "FileIO/FileDialog.h"
#include "Path.h"
#include "Resources.h"
#include "ECS/ECS.h"
#include <vector>
#include <algorithm>

namespace {
  std::vector<Engine::ProjectSaveManager::DirtyState> dirty_states;
}

void Engine::ProjectSaveManager::set_dirty(DirtyState state)
{
  // Check if this state already exists to avoid duplicates
  if (!check_dirty(state)) {
    dirty_states.push_back(std::move(state));
  }
}

bool Engine::ProjectSaveManager::check_dirty(const DirtyState& state)
{
  return std::any_of(dirty_states.begin(), dirty_states.end(),
    [&state](const DirtyState& existing) {
      // Compare based on type and content
      return std::visit([&existing](const auto& s) -> bool {
        using T = std::decay_t<decltype(s)>;

        if (const T* existing_ptr = std::get_if<T>(&existing)) {
          if constexpr (std::is_same_v<T, DirtyStates::ProjectSettings>) {
            return true; // ProjectSettings is always the same
          }
          else if constexpr (std::is_same_v<T, DirtyStates::Level>) {
            return existing_ptr->level_path == s.level_path;
          }
          else if constexpr (std::is_same_v<T, DirtyStates::Prefab>) {
            return existing_ptr->prefab_path == s.prefab_path;
          }
        }
        return false;
        }, state);
    });
}

void Engine::ProjectSaveManager::save_all_dirty()
{
  for (const auto& state : dirty_states) {
    std::visit([](const auto& s) {
      using T = std::decay_t<decltype(s)>;

      if constexpr (std::is_same_v<T, DirtyStates::ProjectSettings>) {
        // Sync collision matrix from ECS to ProjectSettings before saving
        auto& project_settings = Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::get_mutable();
        auto& containers = Engine::ECS::Internal::get_containers();

        Log::info("Saving ProjectSettings - loaded_tilemaps count: " + std::to_string(project_settings.loaded_tilemaps.size()));

        // Convert collision matrix from ID-based to name-based pairs
        project_settings.layer_collision_pairs.clear();

        // Iterate through the collision matrix and save enabled collision pairs
        for (size_t layer_a = 0; layer_a < Engine::ECS::MAX_LAYERS; ++layer_a) {
          for (size_t layer_b = layer_a; layer_b < Engine::ECS::MAX_LAYERS; ++layer_b) {
            // Check if both layers are valid and collision is enabled
            if (layer_a < containers.layer_id_to_name.size() &&
              layer_b < containers.layer_id_to_name.size() &&
              !containers.layer_id_to_name[layer_a].empty() &&
              !containers.layer_id_to_name[layer_b].empty() &&
              containers.collision_matrix[layer_a].test(layer_b)) {

              std::string name_a = containers.layer_id_to_name[layer_a];
              std::string name_b = containers.layer_id_to_name[layer_b];

              // Only store if collision is enabled
              project_settings.layer_collision_pairs.push_back({ name_a, name_b });
            }
          }
        }

        Log::info("Saved " + std::to_string(project_settings.layer_collision_pairs.size()) + " layer collision pairs");

        // Save project settings
        if (Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::save("Assets/settings/project_settings.json"))
        {
          Log::info("Project settings saved.");
        }
        else
        {
          Log::error("Failed to save project settings.");
        }
      }
      else if constexpr (std::is_same_v<T, DirtyStates::Level>) {
        // Save specific level
        std::string level_path = s.level_path;

        // If level path is empty, prompt user for save location
        if (level_path.empty()) {
          Log::info("Current level has no save path - prompting user for location");

          auto selected_path = Engine::FileIO::show_save_file_dialog(
            "Save Level As",
            "untitled.json",
            "Level Files (*.json)\0*.json\0All Files\0*.*\0",
            "json",
            Engine::Resources::resource_path  // Start in the resource folder
          );

          if (!selected_path.has_value()) {
            Log::info("User cancelled save operation");
            return; // User cancelled, don't save
          }

          level_path = selected_path.value();

          // Update the current level path so subsequent saves don't prompt again
          Engine::LevelManager::set_current_level_path(level_path);

          Log::info("User selected save location: " + level_path);
        }
        else {
          Log::info("Saving level: " + level_path);
        }

        // Generate level from current world state
        Engine::Level level = Engine::Level::from_world();

        // Get relative path for saving
        std::string relative_path = Engine::Resources::get_relative_path(level_path);

        // Save the level
        if (Engine::Resources::save_level(relative_path, level, true)) {
          Log::info("Level saved successfully: " + relative_path);
        }
        else {
          Log::error("Failed to save level: " + relative_path);
        }
      }
      else if constexpr (std::is_same_v<T, DirtyStates::Prefab>) {
        // Save specific prefab
        Log::info("Saving prefab: " + s.prefab_path);

        // Construct full path from relative path
        std::string full_path = Engine::Path(Engine::Resources::resource_path).join(s.prefab_path);

        auto it = Engine::Resources::prefab_cache.find(full_path);
        if (it != Engine::Resources::prefab_cache.end()) {
          if (auto prefab = it->second.lock()) {
            Engine::Resources::save_prefab(s.prefab_path, Engine::ECS::Prefab(prefab), true);
          }
          else {
            Log::error("Failed to save prefab - expired in cache: " + s.prefab_path);
          }
        }
        else {
          Log::error("Failed to save prefab - not found in cache: " + s.prefab_path);
        }
      }
      }, state);
  }

  dirty_states.clear();
}

bool Engine::ProjectSaveManager::is_any_dirty()
{
  return !dirty_states.empty();
}
