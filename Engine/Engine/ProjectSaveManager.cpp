#include "ProjectSaveManager.h"
#include "Logger.h"
#include "FileIO/Settings.h"
#include "Path.h"
#include <vector>
#include <algorithm>

namespace {
  std::vector<Engine::ProjectSaveManager::DirtyState> dirty_states;
}

void Engine::ProjectSaveManager::set_dirty(DirtyState state)
{
  // Check if this state already exists to avoid duplicates
  bool already_exists = std::any_of(dirty_states.begin(), dirty_states.end(),
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
          return false;
        }
        return false;
        }, state);
    });

  if (!already_exists) {
    dirty_states.push_back(std::move(state));
  }
}

void Engine::ProjectSaveManager::save_all_dirty()
{
  for (const auto& state : dirty_states) {
    std::visit([](const auto& s) {
      using T = std::decay_t<decltype(s)>;

      if constexpr (std::is_same_v<T, DirtyStates::ProjectSettings>) {
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
        Log::info("Saving level: " + s.level_path);
        
        // Construct full path from relative path
        std::string full_path = Engine::Path(Engine::Resources::resource_path).join(s.level_path);
        
        auto it = Engine::Resources::level_cache.find(full_path);
        if (it != Engine::Resources::level_cache.end()) {
          Engine::Resources::save_level(s.level_path, it->second, true);
        }
        else {
          Log::error("Failed to save level - not found in cache: " + s.level_path);
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

bool Engine::ProjectSaveManager::get_is_any_dirty()
{
  return !dirty_states.empty();
}
