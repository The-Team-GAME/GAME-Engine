//-----------------------------------------------------------------------------------
// file:    LevelTabManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   UI-only tab management for the level editor. Delegates all level
//          operations to LevelManager while maintaining tab display state.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "LevelTabManager.h"
#include "LevelManager.h"
#include "Path.h"
#include "Resources.h"
#include "Logger.h"
#include <algorithm>

namespace {
  std::unordered_map<std::string, Engine::LevelTab> tabs;
  std::vector<std::string> tab_order;
  std::string active_tab_id;
  const std::string empty_string;
  size_t new_tab_counter = 1;

  // Find next available number for new level tabs
  size_t find_next_new_number() {
    size_t num = 1;
    while (true) {
      std::string tab_id = "__new_" + std::to_string(num);
      if (tabs.find(tab_id) == tabs.end()) {
        return num;
      }
      ++num;
    }
  }
}

namespace Engine {
  namespace LevelTabManager {

    bool open_tab(const std::string& filepath, bool append_resource_path)
    {
      // Get the full normalized path - this is the tab ID
      std::string full_path = append_resource_path
        ? Path(Resources::resource_path).join(filepath).normalize().to_string()
        : Path(filepath).normalize().to_string();

      // If tab already exists, just switch to it
      if (tab_exists(full_path)) {
        switch_to_tab(full_path);
        LevelManager::reload_level(full_path);
        tabs[full_path].needs_selection = true;
        return true;
      }

      // Delegate level opening to LevelManager
      std::string level_id = LevelManager::open_level(filepath, append_resource_path);
      if (level_id.empty()) {
        return false;
      }

      // Create tab UI state
      LevelTab new_tab;
      new_tab.display_name = Path(full_path).get_filename();
      new_tab.is_dirty = false;
      new_tab.is_new = false;
      new_tab.needs_selection = true;

      tabs[full_path] = std::move(new_tab);
      tab_order.push_back(full_path);
      active_tab_id = full_path;

      return true;
    }

    std::string create_new_tab(const std::string& name)
    {
      // Delegate level creation to LevelManager
      std::string level_id = LevelManager::create_new_level();
      if (level_id.empty()) {
        return "";
      }

      // Find next number for display name
      size_t next_num = find_next_new_number();
      std::string display_name = name + " " + std::to_string(next_num);

      // Create tab UI state
      LevelTab new_tab;
      new_tab.display_name = display_name;
      new_tab.is_dirty = true;
      new_tab.is_new = true;
      new_tab.needs_selection = true;

      tabs[level_id] = std::move(new_tab);
      tab_order.push_back(level_id);
      active_tab_id = level_id;

      return level_id;
    }

    void switch_to_tab(const std::string& tab_id)
    {
      if (!tab_exists(tab_id)) {
        Log::warning("LevelTabManager: Tab not found: {}", tab_id);
        return;
      }

      if (tab_id == active_tab_id) {
        return;
      }

      // Delegate to LevelManager
      LevelManager::switch_to_level(tab_id);
      active_tab_id = tab_id;
    }

    bool close_tab(const std::string& tab_id, bool force)
    {
      if (!tab_exists(tab_id)) {
        return false;
      }

      LevelTab& tab = tabs[tab_id];

      // Check dirty state
      if (!force && tab.is_dirty) {
        Log::warning("Tab '{}' has unsaved changes", tab.display_name);
        return false;
      }

      // Find position
      auto it = std::find(tab_order.begin(), tab_order.end(), tab_id);
      size_t index = std::distance(tab_order.begin(), it);

      // Delegate level closing to LevelManager
      LevelManager::close_level(tab_id);

      // Update active tab
      if (tab_id == active_tab_id) {
        if (tab_order.size() > 1) {
          size_t new_index = (index > 0) ? index - 1 : 1;
          active_tab_id = tab_order[new_index];
        }
        else {
          active_tab_id = "";
        }
      }

      // Remove tab
      tabs.erase(tab_id);
      tab_order.erase(it);

      return true;
    }

    bool save_tab(const std::string& tab_id)
    {
      if (!tab_exists(tab_id)) {
        return false;
      }

      if (tab_id.starts_with("__new_")) {
        Log::warning("Cannot save new tab without path. Use save_tab_as.");
        return false;
      }

      // Delegate to LevelManager
      if (LevelManager::save_level(tab_id)) {
        tabs[tab_id].is_dirty = false;
        tabs[tab_id].is_new = false;
        return true;
      }

      return false;
    }

    std::string save_tab_as(const std::string& tab_id, const std::string& filepath, bool append_resource_path)
    {
      if (!tab_exists(tab_id)) {
        return "";
      }

      // Delegate to LevelManager
      std::string new_level_id = LevelManager::save_level_as(tab_id, filepath, append_resource_path);
      if (new_level_id.empty()) {
        return "";
      }

      // Move tab to new ID
      LevelTab old_tab = std::move(tabs[tab_id]);
      old_tab.display_name = Path(new_level_id).get_filename();
      old_tab.is_dirty = false;
      old_tab.is_new = false;

      auto it = std::find(tab_order.begin(), tab_order.end(), tab_id);
      *it = new_level_id;

      tabs.erase(tab_id);
      tabs[new_level_id] = std::move(old_tab);

      if (active_tab_id == tab_id) {
        active_tab_id = new_level_id;
      }

      return new_level_id;
    }

    void mark_active_dirty()
    {
      if (!active_tab_id.empty() && tab_exists(active_tab_id)) {
        tabs[active_tab_id].is_dirty = true;
      }
    }

    const std::unordered_map<std::string, LevelTab>& get_tabs()
    {
      return tabs;
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
      if (active_tab_id.empty()) return nullptr;
      return get_tab(active_tab_id);
    }

    LevelTab* get_tab(const std::string& tab_id)
    {
      auto it = tabs.find(tab_id);
      return (it != tabs.end()) ? &it->second : nullptr;
    }

    bool has_unsaved_changes()
    {
      return std::any_of(tabs.begin(), tabs.end(),
        [](const auto& pair) { return pair.second.is_dirty; });
    }

    bool tab_exists(const std::string& tab_id)
    {
      return tabs.find(tab_id) != tabs.end();
    }

    size_t get_tab_count()
    {
      return tabs.size();
    }

    void clear_all()
    {
      // Delegate to LevelManager
      LevelManager::clear_all();

      // Clear tab state
      tabs.clear();
      tab_order.clear();
      active_tab_id = "";
    }

  } // namespace LevelTabManager
} // namespace Engine

