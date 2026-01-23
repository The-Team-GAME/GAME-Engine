//-----------------------------------------------------------------------------------
// file:    LevelTabManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the LevelTabManager for managing multiple open level
//          tabs. Each tab stores a cached version of the level data, allowing users
//          to switch between levels without losing unsaved changes.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "Level.h"

namespace Engine {

  /*=================================================================================
  Struct Brief:
  LevelTab - Represents a single open level tab with its cached data and state.

  Members:
  - display_name: Tab title shown in UI
  - cached_level: Level data when tab is not active (empty when active)
  - is_dirty: Whether the level has unsaved changes
  - is_new: Whether this is a new unsaved level (no file on disk yet)
  ==================================================================================*/
  struct LevelTab {
    std::string display_name;
    Level cached_level;
    bool is_dirty = false;
    bool is_new = false;
    bool needs_selection = false;  // Used to auto-select tab in UI
  };

  namespace LevelTabManager {

    // Opens a level in a new tab. If already open, switches to that tab.
    bool open_level_tab(const std::string& filepath, bool append_resource_path = true);

    // Creates a new empty level tab. Returns the tab ID.
    std::string create_new_level_tab(const std::string& name = "New Level");

    // Switches to a tab by its ID (file path or generated key).
    void switch_to_tab(const std::string& tab_id);

    // Closes a tab. Returns false if dirty and force is false.
    bool close_tab(const std::string& tab_id, bool force = false);

    // Saves the tab to its file path. Returns false if no path set.
    bool save_tab(const std::string& tab_id);

    // Saves tab to new path. Returns new tab ID on success, empty on failure.
    std::string save_tab_as(const std::string& tab_id, const std::string& filepath, bool append_resource_path = true);

    // Marks the active tab as dirty.
    void mark_active_dirty();

    // Gets all tabs (key = tab_id).
    const std::unordered_map<std::string, LevelTab>& get_tabs();

    // Gets tab IDs in display order.
    const std::vector<std::string>& get_tab_order();

    // Gets the active tab ID.
    const std::string& get_active_tab_id();

    // Gets the active tab, or nullptr if none.
    LevelTab* get_active_tab();

    // Gets a specific tab by ID, or nullptr if not found.
    LevelTab* get_tab(const std::string& tab_id);

    // Returns true if any tab has unsaved changes.
    bool has_unsaved_changes();

    // Returns true if tab with given ID exists.
    bool tab_exists(const std::string& tab_id);

    // Caches the current world into the active tab.
    void cache_active_tab();

    // Clears all tabs.
    void clear_all();

    // Gets total tab count.
    size_t get_tab_count();

  } // namespace LevelTabManager

} // namespace Engine
