//-----------------------------------------------------------------------------------
// file:    LevelTabManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   UI-only tab management for the level editor. Stores tab display state
//          (dirty flags, display names, selection state) while delegating all
//          level logic to LevelManager.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace Engine {

  /*=================================================================================
  Struct Brief:
  LevelTab - UI state for a level tab in the editor.
  ==================================================================================*/
  struct LevelTab {
    std::string display_name;
    bool is_dirty = false;
    bool is_new = false;
    bool needs_selection = false;  // Flag to auto-select in ImGui
  };

  namespace LevelTabManager {

    // ============================================================================
    // Tab Operations (delegates to LevelManager)
    // ============================================================================

    // Opens a level and creates a tab for it.
    bool open_tab(const std::string& filepath, bool append_resource_path = true);

    // Creates a new empty level tab.
    std::string create_new_tab(const std::string& name = "New Level");

    // Switches to a tab (and its level).
    void switch_to_tab(const std::string& tab_id);

    // Closes a tab. Returns false if dirty and force is false.
    bool close_tab(const std::string& tab_id, bool force = false);

    // Saves the tab's level. Returns false for new levels.
    bool save_tab(const std::string& tab_id);

    // Saves the tab's level to a new path. Returns new tab ID.
    std::string save_tab_as(const std::string& tab_id, const std::string& filepath, bool append_resource_path = true);

    // ============================================================================
    // Tab State
    // ============================================================================

    // Marks the active tab as dirty.
    void mark_active_dirty();

    // Gets all tabs.
    const std::unordered_map<std::string, LevelTab>& get_tabs();

    // Gets tab IDs in display order.
    const std::vector<std::string>& get_tab_order();

    // Gets the active tab ID.
    const std::string& get_active_tab_id();

    // Gets the active tab, or nullptr if none.
    LevelTab* get_active_tab();

    // Gets a specific tab by ID.
    LevelTab* get_tab(const std::string& tab_id);

    // Returns true if any tab has unsaved changes.
    bool has_unsaved_changes();

    // Returns true if tab exists.
    bool tab_exists(const std::string& tab_id);

    // Gets total tab count.
    size_t get_tab_count();

    // Clears all tabs.
    void clear_all();

  } // namespace LevelTabManager

} // namespace Engine

