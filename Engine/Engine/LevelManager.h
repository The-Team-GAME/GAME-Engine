//-----------------------------------------------------------------------------------
// file:    LevelManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Central level management system. Handles level loading, saving, caching,
//          switching, and lifecycle callbacks. Provides a unified interface for
//          both editor and game mode level operations.
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
    LevelState - Stores the complete state of an open level including cached data
                 and lifecycle callbacks.
    ==================================================================================*/
    struct LevelState {
        Level cached_level;           // Level data when not active (empty when active)
        SerializableFunction on_init;
        SerializableFunction on_update;
        SerializableFunction on_exit;
    };

    namespace LevelManager {

        // ============================================================================
        // Level Operations
        // ============================================================================

        // Opens a level from disk. Returns the level ID (normalized path).
        // If already open, switches to it. Calls on_exit for previous and on_init for new.
        std::string open_level(const std::string& filepath, bool append_resource_path = true);

        // Creates a new empty level. Returns the level ID (__new_X format).
        std::string create_new_level();

        // Switches to an already-open level. Calls on_exit/on_init callbacks.
        void switch_to_level(const std::string& level_id);

        // Closes a level. Returns false if it doesn't exist.
        // Switches to another open level if closing the active one.
        bool close_level(const std::string& level_id);

        // Saves the level to its current path. Returns false for new unsaved levels.
        bool save_level(const std::string& level_id);

        // Saves the level to a new path. Returns the new level ID, or empty on failure.
        std::string save_level_as(const std::string& level_id, const std::string& filepath, bool append_resource_path = true);

        // ============================================================================
        // Level State Access
        // ============================================================================

        // Gets the current level path (empty for new unsaved levels).
        const std::string& get_current_level_path();

        // Gets the active level ID (may differ from path for new levels).
        const std::string& get_active_level_id();

        // Gets the state for a specific level, or nullptr if not found.
        LevelState* get_level_state(const std::string& level_id);

        // Gets the active level state, or nullptr if none.
        LevelState* get_active_level_state();

        // Gets all open level IDs in order.
        const std::vector<std::string>& get_open_level_ids();

        // Returns true if the specified level is open.
        bool is_level_open(const std::string& level_id);

        // Gets total number of open levels.
        size_t get_open_level_count();

        // ============================================================================
        // Lifecycle Callbacks
        // ============================================================================

        // Calls the on_update callback for the active level (call in game loop).
        void call_active_update();

        // ============================================================================
        // World/Cache Management
        // ============================================================================

        // Caches the current world state into the active level.
        void cache_active_level();

        // Gets a Level struct with current world entities + callbacks from active level.
        Level get_level_for_save(const std::string& level_id);

        // Clears all open levels and the world.
        void clear_all();

        // ============================================================================
        // Internal (used by LevelTabManager)
        // ============================================================================

        // Sets the current level path directly (for compatibility).
        void set_current_level_path(const std::string& path);

    } // namespace LevelManager

} // namespace Engine
