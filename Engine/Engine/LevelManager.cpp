//-----------------------------------------------------------------------------------
// file:    LevelManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implements the central level management system. Handles level loading,
//          saving, caching, switching, and lifecycle callbacks.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "LevelManager.h"
#include "Resources.h"
#include "Path.h"
#include "Logger.h"
#include "ECS/ECS.h"
#include "FileIO/LevelFileIO.h"
#include "EventManager/EventManager.h"
#include "Events/EngineEvents.h"
#include <algorithm>

namespace {
  std::unordered_map<std::string, Engine::LevelState> open_levels;
  std::vector<std::string> level_order;  // Maintains order of open levels
  std::string active_level_id;
  std::string current_level_path;  // Actual file path (empty for new levels)
  const std::string empty_string;

  // Find the next available number for new levels
  size_t find_next_new_level_number() {
    size_t num = 1;
    while (true) {
      std::string level_id = "__new_" + std::to_string(num);
      if (open_levels.find(level_id) == open_levels.end()) {
        return num;
      }
      ++num;
    }
  }

  // Call on_exit for a level if it has one
  void call_level_exit(const std::string& level_id) {
    auto it = open_levels.find(level_id);
    if (it == open_levels.end()) return;

    Engine::LevelState& state = it->second;
    if (!state.on_exit.empty()) {
      auto func = state.on_exit.get<Engine::LevelCallback>();
      if (func) {
        func();
      }
      else {
        Log::warning("Level on_exit callback '{}' not found in FunctionRegistry", state.on_exit.name);
      }
    }
  }

  // Call on_init for a level if it has one
  void call_level_init(const std::string& level_id) {
    auto it = open_levels.find(level_id);
    if (it == open_levels.end()) return;

    Engine::LevelState& state = it->second;
    if (!state.on_init.empty()) {
      auto func = state.on_init.get<Engine::LevelCallback>();
      if (func) {
        func();
      }
      else {
        Log::warning("Level on_init callback '{}' not found in FunctionRegistry", state.on_init.name);
      }
    }
  }
}

namespace Engine {
  namespace LevelManager {

    std::string open_level(const std::string& filepath, bool append_resource_path)
    {
      // Get the full normalized path - this becomes the level ID
      std::string full_path = append_resource_path
        ? Path(Resources::resource_path).join(filepath).normalize().to_string()
        : Path(filepath).normalize().to_string();

      // Check if this level is already open
      if (is_level_open(full_path)) {
        switch_to_level(full_path);
        Log::info("Switched to existing level: {}", full_path);
        return full_path;
      }

      // Call on_exit for current level before switching
      if (!active_level_id.empty() && is_level_open(active_level_id)) {
        call_level_exit(active_level_id);
        cache_active_level();
      }

      // Load the level from disk
      Level level = Resources::load_level(filepath, append_resource_path, false);

      // Create a new level state
      LevelState new_state;
      new_state.cached_level = Level();  // Empty - entities go into world
      new_state.on_init = level.on_init;
      new_state.on_update = level.on_update;
      new_state.on_exit = level.on_exit;

      open_levels[full_path] = std::move(new_state);
      level_order.push_back(full_path);
      active_level_id = full_path;
      current_level_path = full_path;

      // Instantiate into world
      level.instantiate_into_world(true);

      // Call on_init callback
      call_level_init(full_path);

      Log::info("Opened level: {}", full_path);
      EventManager::send_event<Events::ToastEvent>(
        "Opened: " + Path(full_path).get_filename(),
        Events::ToastType::Info,
        2.0f
      );

      return full_path;
    }

    std::string create_new_level()
    {
      // Call on_exit for current level before creating new one
      if (!active_level_id.empty() && is_level_open(active_level_id)) {
        call_level_exit(active_level_id);
        cache_active_level();
      }

      // Find the next available number
      size_t next_num = find_next_new_level_number();
      std::string level_id = "__new_" + std::to_string(next_num);

      // Create a new empty level state
      LevelState new_state;
      new_state.cached_level = Level();

      open_levels[level_id] = std::move(new_state);
      level_order.push_back(level_id);
      active_level_id = level_id;
      current_level_path = "";  // No file path for new levels

      // Clear the world
      ECS::for_each_entity([](ECS::Entity entity) {
        entity.destroy();
        });
      ECS::process_housekeeping();

      Log::info("Created new level: {}", level_id);

      return level_id;
    }

    void switch_to_level(const std::string& level_id)
    {
      if (!is_level_open(level_id)) {
        Log::warning("Cannot switch to level - not open: {}", level_id);
        return;
      }

      if (level_id == active_level_id) {
        return;  // Already active
      }

      // Call on_exit for current level
      if (!active_level_id.empty() && is_level_open(active_level_id)) {
        call_level_exit(active_level_id);
        cache_active_level();
      }

      // Switch to new level
      active_level_id = level_id;
      current_level_path = level_id.starts_with("__new_") ? "" : level_id;

      LevelState& state = open_levels[level_id];

      // Instantiate cached level into world
      state.cached_level.instantiate_into_world(true);
      state.cached_level = Level();  // Clear cache

      // Call on_init
      call_level_init(level_id);

      Log::info("Switched to level: {}", level_id);
    }

    bool close_level(const std::string& level_id)
    {
      if (!is_level_open(level_id)) {
        Log::warning("Cannot close level - not open: {}", level_id);
        return false;
      }

      // Call on_exit if closing active level
      if (level_id == active_level_id) {
        call_level_exit(level_id);
      }

      // Find position in order
      auto it = std::find(level_order.begin(), level_order.end(), level_id);
      size_t index = std::distance(level_order.begin(), it);

      // If closing active level, switch to another
      if (level_id == active_level_id) {
        if (level_order.size() > 1) {
          size_t new_index = (index > 0) ? index - 1 : 1;
          std::string new_active = level_order[new_index];

          LevelState& state = open_levels[new_active];
          current_level_path = new_active.starts_with("__new_") ? "" : new_active;
          state.cached_level.instantiate_into_world(true);
          state.cached_level = Level();
          active_level_id = new_active;

          call_level_init(new_active);
        }
        else {
          // Closing last level
          ECS::for_each_entity([](ECS::Entity entity) {
            entity.destroy();
            });
          ECS::process_housekeeping();
          active_level_id = "";
          current_level_path = "";
        }
      }

      // Remove from containers
      open_levels.erase(level_id);
      level_order.erase(it);

      Log::info("Closed level: {}", level_id);
      return true;
    }

    bool save_level(const std::string& level_id)
    {
      if (!is_level_open(level_id)) {
        Log::warning("Cannot save level - not open: {}", level_id);
        return false;
      }

      if (level_id.starts_with("__new_")) {
        Log::warning("Cannot save new level without path. Use save_level_as instead.");
        return false;
      }

      Level level_to_save = get_level_for_save(level_id);

      if (Resources::save_level(level_id, level_to_save, false)) {
        Log::info("Saved level: {}", level_id);
        EventManager::send_event<Events::ToastEvent>(
          "Saved: " + Path(level_id).get_filename(),
          Events::ToastType::Info,
          2.0f
        );
        return true;
      }

      Log::error("Failed to save level: {}", level_id);
      return false;
    }

    std::string save_level_as(const std::string& level_id, const std::string& filepath, bool append_resource_path)
    {
      if (!is_level_open(level_id)) {
        Log::warning("Cannot save level - not open: {}", level_id);
        return "";
      }

      // Get the new path
      std::string new_level_id = append_resource_path
        ? Path(Resources::resource_path).join(filepath).normalize().to_string()
        : Path(filepath).normalize().to_string();

      Level level_to_save = get_level_for_save(level_id);

      if (!Resources::save_level(new_level_id, level_to_save, false)) {
        Log::error("Failed to save level as: {}", new_level_id);
        return "";
      }

      // Move state to new ID
      LevelState old_state = std::move(open_levels[level_id]);

      auto it = std::find(level_order.begin(), level_order.end(), level_id);
      *it = new_level_id;

      open_levels.erase(level_id);
      open_levels[new_level_id] = std::move(old_state);

      if (active_level_id == level_id) {
        active_level_id = new_level_id;
        current_level_path = new_level_id;
      }

      Log::info("Saved level as: {}", new_level_id);
      EventManager::send_event<Events::ToastEvent>(
        "Saved as: " + Path(new_level_id).get_filename(),
        Events::ToastType::Info,
        2.0f
      );

      return new_level_id;
    }

    const std::string& get_current_level_path()
    {
      return current_level_path;
    }

    const std::string& get_active_level_id()
    {
      return active_level_id.empty() ? empty_string : active_level_id;
    }

    LevelState* get_level_state(const std::string& level_id)
    {
      auto it = open_levels.find(level_id);
      return (it != open_levels.end()) ? &it->second : nullptr;
    }

    LevelState* get_active_level_state()
    {
      if (active_level_id.empty()) return nullptr;
      return get_level_state(active_level_id);
    }

    const std::vector<std::string>& get_open_level_ids()
    {
      return level_order;
    }

    bool is_level_open(const std::string& level_id)
    {
      return open_levels.find(level_id) != open_levels.end();
    }

    size_t get_open_level_count()
    {
      return open_levels.size();
    }

    void call_active_update()
    {
      if (active_level_id.empty()) return;

      auto it = open_levels.find(active_level_id);
      if (it == open_levels.end()) return;

      LevelState& state = it->second;
      if (!state.on_update.empty()) {
        auto func = state.on_update.get<LevelCallback>();
        if (func) {
          func();
        }
      }
    }

    void cache_active_level()
    {
      if (active_level_id.empty()) return;

      auto it = open_levels.find(active_level_id);
      if (it == open_levels.end()) return;

      LevelState& state = it->second;
      state.cached_level = Level::from_world();

      // Preserve callbacks in cached level
      state.cached_level.on_init = state.on_init;
      state.cached_level.on_update = state.on_update;
      state.cached_level.on_exit = state.on_exit;

      Log::info("Cached level: {} ({} entities)",
        active_level_id,
        state.cached_level.entities.size());
    }

    Level get_level_for_save(const std::string& level_id)
    {
      Level level;

      auto it = open_levels.find(level_id);
      if (it == open_levels.end()) {
        return level;
      }

      LevelState& state = it->second;

      // Get entities from world if active, otherwise from cache
      if (level_id == active_level_id) {
        level = Level::from_world();
      }
      else {
        level = state.cached_level;
      }

      // Always restore callbacks from state
      level.on_init = state.on_init;
      level.on_update = state.on_update;
      level.on_exit = state.on_exit;

      return level;
    }

    void clear_all()
    {
      // Call on_exit for active level
      if (!active_level_id.empty() && is_level_open(active_level_id)) {
        call_level_exit(active_level_id);
      }

      open_levels.clear();
      level_order.clear();
      active_level_id = "";
      current_level_path = "";

      ECS::for_each_entity([](ECS::Entity entity) {
        entity.destroy();
        });
      ECS::process_housekeeping();

      Log::info("Cleared all levels");
    }

    void set_current_level_path(const std::string& path)
    {
      current_level_path = path;
    }

  } // namespace LevelManager
} // namespace Engine
