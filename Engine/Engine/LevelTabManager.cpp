//-----------------------------------------------------------------------------------
// file:    LevelTabManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the LevelTabManager for managing multiple open level
//          tabs with virtual world caching for seamless tab switching.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "LevelTabManager.h"
#include "Resources.h"
#include "Path.h"
#include "Logger.h"
#include "ECS/ECS.h"
#include "FileIO/LevelFileIO.h"
#include "EventManager/EventManager.h"
#include "Events/EngineEvents.h"
#include <algorithm>
#include <filesystem>

namespace {
  std::unordered_map<std::string, Engine::LevelTab> open_tabs;
  std::vector<std::string> tab_order; // Maintains display order
  std::string active_tab_id;
  const std::string empty_string;

  // Find the next available number for new levels
  size_t find_next_new_level_number() {
    size_t num = 1;
    while (true) {
      std::string tab_id = "__new_" + std::to_string(num);
      if (open_tabs.find(tab_id) == open_tabs.end()) {
        return num;
      }
      ++num;
    }
  }
}

namespace Engine {
  namespace LevelTabManager {

    bool open_level_tab(const std::string& filepath, bool append_resource_path)
    {
      // Get the full normalized path - this becomes the tab ID
      std::string full_path = append_resource_path
        ? Path(Resources::resource_path).join(filepath).normalize().to_string()
        : Path(filepath).normalize().to_string();

      // Check if this level is already open
      if (tab_exists(full_path)) {
        switch_to_tab(full_path);
        open_tabs[full_path].needs_selection = true;  // Auto-select in UI
        Log::info("Switched to existing tab for level: {}", full_path);
        return true;
      }

      // Cache the current active tab before opening a new one
      if (!active_tab_id.empty()) {
        cache_active_tab();
      }

      // Load the level from disk
      Level level = Resources::load_level(filepath, append_resource_path, false);

      // Create a new tab
      LevelTab new_tab;
      new_tab.display_name = Path(full_path).get_filename();
      new_tab.is_dirty = false;
      new_tab.is_new = false;
      new_tab.cached_level = Level(); // Empty - will be loaded into world
      new_tab.needs_selection = true;  // Auto-select in UI

      open_tabs[full_path] = std::move(new_tab);
      tab_order.push_back(full_path);
      active_tab_id = full_path;

      // Set the current level path and instantiate into world
      LevelManager::set_current_level_path(full_path);
      level.instantiate_into_world(true);

      Log::info("Opened level in new tab: {}", full_path);
      EventManager::send_event<Events::ToastEvent>(
        "Opened: " + open_tabs[full_path].display_name,
        Events::ToastType::Info,
        2.0f
      );

      return true;
    }

    std::string create_new_level_tab(const std::string& name)
    {
      // Cache the current active tab before creating a new one
      if (!active_tab_id.empty()) {
        cache_active_tab();
      }

      // Find the next available number for new levels
      size_t next_num = find_next_new_level_number();

      // Generate a unique tab ID for new unsaved levels
      std::string tab_id = "__new_" + std::to_string(next_num);
      std::string display_name = name + " " + std::to_string(next_num);

      // Create a new tab
      LevelTab new_tab;
      new_tab.display_name = display_name;
      new_tab.is_dirty = true;
      new_tab.is_new = true;
      new_tab.cached_level = Level();
      new_tab.needs_selection = true;  // Mark for selection in UI

      open_tabs[tab_id] = std::move(new_tab);
      tab_order.push_back(tab_id);
      active_tab_id = tab_id;

      // Clear the current level path and clear the world
      LevelManager::set_current_level_path("");

      ECS::for_each_entity([](ECS::Entity entity) {
        entity.destroy();
        });
      ECS::process_housekeeping();

      Log::info("Created new level tab: {}", display_name);

      return tab_id;
    }

    void switch_to_tab(const std::string& tab_id)
    {
      if (!tab_exists(tab_id)) {
        Log::warning("Invalid tab ID: {}", tab_id);
        return;
      }

      if (tab_id == active_tab_id) {
        return; // Already on this tab
      }

      // Cache the current tab's world state
      cache_active_tab();

      // Switch to the new tab
      active_tab_id = tab_id;
      LevelTab& target_tab = open_tabs[tab_id];

      // Update the current level path (empty for new unsaved levels)
      std::string level_path = tab_id.starts_with("__new_") ? "" : tab_id;
      LevelManager::set_current_level_path(level_path);

      // Instantiate the cached level into the world
      target_tab.cached_level.instantiate_into_world(true);

      // Clear the cache since it's now in the world
      target_tab.cached_level = Level();

      Log::info("Switched to tab: {}", target_tab.display_name);
    }

    bool close_tab(const std::string& tab_id, bool force)
    {
      if (!tab_exists(tab_id)) {
        Log::warning("Invalid tab ID for closing: {}", tab_id);
        return false;
      }

      LevelTab& tab = open_tabs[tab_id];

      // Check for unsaved changes
      if (!force && tab.is_dirty) {
        Log::warning("Tab '{}' has unsaved changes", tab.display_name);
        return false;
      }

      std::string closed_name = tab.display_name;

      // Find position in tab_order
      auto it = std::find(tab_order.begin(), tab_order.end(), tab_id);
      size_t index = std::distance(tab_order.begin(), it);

      // If closing the active tab, switch to another tab first
      if (tab_id == active_tab_id) {
        if (tab_order.size() > 1) {
          // Switch to adjacent tab
          size_t new_index = (index > 0) ? index - 1 : 1;
          std::string new_active = tab_order[new_index];

          LevelTab& target_tab = open_tabs[new_active];
          std::string level_path = new_active.starts_with("__new_") ? "" : new_active;
          LevelManager::set_current_level_path(level_path);
          target_tab.cached_level.instantiate_into_world(true);
          target_tab.cached_level = Level();
          active_tab_id = new_active;
        }
        else {
          // Closing the last tab - clear the world
          ECS::for_each_entity([](ECS::Entity entity) {
            entity.destroy();
            });
          ECS::process_housekeeping();
          LevelManager::set_current_level_path("");
          active_tab_id = "";
        }
      }

      // Remove from both containers
      open_tabs.erase(tab_id);
      tab_order.erase(it);

      Log::info("Closed tab: {}", closed_name);
      return true;
    }

    bool save_tab(const std::string& tab_id)
    {
      if (!tab_exists(tab_id)) {
        Log::warning("Invalid tab ID for saving: {}", tab_id);
        return false;
      }

      // Can't save a new level without a path
      if (tab_id.starts_with("__new_")) {
        Log::warning("Cannot save new level without path. Use save_tab_as instead.");
        return false;
      }

      LevelTab& tab = open_tabs[tab_id];

      // Get the level data to save
      Level level_to_save;
      if (tab_id == active_tab_id) {
        level_to_save = Level::from_world();
      }
      else {
        level_to_save = tab.cached_level;
      }

      // Save to disk (tab_id is the file path)
      if (Resources::save_level(tab_id, level_to_save, false)) {
        tab.is_dirty = false;
        tab.is_new = false;
        Log::info("Saved tab: {}", tab.display_name);
        EventManager::send_event<Events::ToastEvent>(
          "Saved: " + tab.display_name,
          Events::ToastType::Info,
          2.0f
        );
        return true;
      }

      Log::error("Failed to save tab: {}", tab.display_name);
      return false;
    }

    std::string save_tab_as(const std::string& tab_id, const std::string& filepath, bool append_resource_path)
    {
      if (!tab_exists(tab_id)) {
        Log::warning("Invalid tab ID for save as: {}", tab_id);
        return "";
      }

      // Get the full normalized path - this becomes the new tab ID
      std::string new_tab_id = append_resource_path
        ? Path(Resources::resource_path).join(filepath).normalize().to_string()
        : Path(filepath).normalize().to_string();

      LevelTab& old_tab = open_tabs[tab_id];

      // Get the level data to save
      Level level_to_save;
      if (tab_id == active_tab_id) {
        level_to_save = Level::from_world();
      }
      else {
        level_to_save = old_tab.cached_level;
      }

      // Save to disk
      if (!Resources::save_level(new_tab_id, level_to_save, false)) {
        Log::error("Failed to save tab as: {}", new_tab_id);
        return "";
      }

      // Create new tab entry with updated info
      LevelTab new_tab;
      new_tab.display_name = Path(new_tab_id).get_filename();
      new_tab.is_dirty = false;
      new_tab.is_new = false;
      new_tab.cached_level = old_tab.cached_level;

      // Replace old tab with new one in the same position
      auto it = std::find(tab_order.begin(), tab_order.end(), tab_id);
      *it = new_tab_id;

      open_tabs.erase(tab_id);
      open_tabs[new_tab_id] = std::move(new_tab);

      // Update active tab if needed
      if (active_tab_id == tab_id) {
        active_tab_id = new_tab_id;
        LevelManager::set_current_level_path(new_tab_id);
      }

      Log::info("Saved tab as: {}", new_tab_id);
      EventManager::send_event<Events::ToastEvent>(
        "Saved as: " + open_tabs[new_tab_id].display_name,
        Events::ToastType::Info,
        2.0f
      );

      return new_tab_id;
    }

    void mark_active_dirty()
    {
      if (!active_tab_id.empty() && tab_exists(active_tab_id)) {
        open_tabs[active_tab_id].is_dirty = true;
      }
    }

    const std::unordered_map<std::string, LevelTab>& get_tabs()
    {
      return open_tabs;
    }

    const std::vector<std::string>& get_tab_order()
    {
      return tab_order;
    }

    const std::string& get_active_tab_id()
    {
      return active_tab_id.empty() ? empty_string : active_tab_id;
    }

    LevelTab* get_active_tab()
    {
      if (active_tab_id.empty() || !tab_exists(active_tab_id)) {
        return nullptr;
      }
      return &open_tabs[active_tab_id];
    }

    LevelTab* get_tab(const std::string& tab_id)
    {
      auto it = open_tabs.find(tab_id);
      return (it != open_tabs.end()) ? &it->second : nullptr;
    }

    bool has_unsaved_changes()
    {
      return std::any_of(open_tabs.begin(), open_tabs.end(),
        [](const auto& pair) { return pair.second.is_dirty; });
    }

    bool tab_exists(const std::string& tab_id)
    {
      return open_tabs.find(tab_id) != open_tabs.end();
    }

    void cache_active_tab()
    {
      if (active_tab_id.empty() || !tab_exists(active_tab_id)) {
        return;
      }

      LevelTab& active = open_tabs[active_tab_id];
      active.cached_level = Level::from_world();

      Log::info("Cached active tab: {} ({} entities)",
        active.display_name,
        active.cached_level.entities.size());
    }

    void clear_all()
    {
      open_tabs.clear();
      tab_order.clear();
      active_tab_id = "";

      ECS::for_each_entity([](ECS::Entity entity) {
        entity.destroy();
        });
      ECS::process_housekeeping();
      LevelManager::set_current_level_path("");

      Log::info("Cleared all level tabs");
    }

    size_t get_tab_count()
    {
      return open_tabs.size();
    }

  } // namespace LevelTabManager
} // namespace Engine
