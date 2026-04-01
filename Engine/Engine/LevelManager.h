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
#include "EngineExport.h"
#include "Reflection/ReflectionLibrary/GenericValue.h"
#include "ECS/Types.h"

namespace Engine {

  // Entities are stored as a vector of GenericValue for serialization/deserialization
  using EntityList = std::vector<ReflectionLibrary::GenericValue>;

  /*=================================================================================
  Struct Brief:
  LevelState - Stores the cached state of an open level.
  ==================================================================================*/
  struct LevelState {
    ECS::WorldID world_id = ECS::INVALID_WORLD;      // World containing this level's entities
    EntityList cached_entities;                        // Serialized entity data (editor cold-load path)
    std::vector<ECS::EntityID> live_entity_ids;        // Deactivated entity IDs (editor fast-switch path)
    bool has_live_entities = false;                     // True if entities are alive but deactivated (editor)
    EntityList preloaded_entities;                     // Deserialized entity data for cold-path reload
    bool preload_ready = false;                        // Preloaded entity data is ready
    bool preload_in_progress = false;                  // Background preload refresh running
    bool preload_refresh_queued = false;               // Another refresh requested while running
    bool keep_loaded = false;                          // Keep this level resident until manually unloaded
  };

  namespace LevelManager {

    // ============================================================================
    // Level Operations
    // ============================================================================

    // Opens a level from disk. Returns the level ID (normalized path).
    // If already open, switches to it. Calls on_exit for previous and on_init for new.
    ENGINE_API std::string open_level(const std::string& filepath, bool append_resource_path = true);

    // Opens a level as a runtime-only overlay without deactivating the active level.
    // Returns an overlay handle that can be passed to close_overlay_level().
    ENGINE_API std::string open_overlay_level(const std::string& filepath, bool append_resource_path = true, bool mark_runtime_only = true);

    // Preloads a level from disk into the ECS world as inactive entities.
    // Returns the level ID (normalized path). Preloaded levels stay resident until closed.
    ENGINE_API std::string preload_level(const std::string& filepath, bool append_resource_path = true);

    // Creates a new empty level. Returns the level ID (__new_X format).
    ENGINE_API std::string create_new_level();

    // Switches to an already-open level. Calls on_exit/on_init callbacks.
    ENGINE_API void switch_to_level(const std::string& level_id);

    // Reloads a level from its preloaded snapshot (or disk fallback).
    ENGINE_API void reload_level(const std::string& level_id);

    // Closes a level. Returns false if it doesn't exist.
    // Switches to another open level if closing the active one.
    ENGINE_API bool close_level(const std::string& level_id);

    // Closes a previously opened overlay level.
    ENGINE_API bool close_overlay_level(const std::string& overlay_id);

    // Saves the level to its current path. Returns false for new unsaved levels.
    ENGINE_API bool save_level(const std::string& level_id);

    // Saves the level to a new path. Returns the new level ID, or empty on failure.
    ENGINE_API std::string save_level_as(const std::string& level_id, const std::string& filepath, bool append_resource_path = true);

    // ============================================================================
    // Level State Access
    // ============================================================================

    // Gets the current level path (empty for new unsaved levels).
    ENGINE_API const std::string& get_current_level_path();

    // Gets the active level ID (may differ from path for new levels).
    ENGINE_API const std::string& get_active_level_id();

    // Gets the state for a specific level, or nullptr if not found.
    ENGINE_API LevelState* get_level_state(const std::string& level_id);

    // Gets the active level state, or nullptr if none.
    ENGINE_API LevelState* get_active_level_state();

    // Gets all open level IDs in order.
    ENGINE_API const std::vector<std::string>& get_open_level_ids();

    // Returns true if the specified level is open.
    ENGINE_API bool is_level_open(const std::string& level_id);

    // Gets total number of open levels.
    ENGINE_API size_t get_open_level_count();

    // ============================================================================
    // Preloaded Snapshot Management
    // ============================================================================

    // Returns true if a preloaded snapshot exists for the given level path.
    ENGINE_API bool has_preloaded_snapshot(const std::string& level_id);

    // Clears the preloaded snapshot for a level, destroying its deactivated entities.
    ENGINE_API void clear_preloaded_snapshot(const std::string& level_id);

    // ============================================================================
    // Lifecycle Callbacks
    // ============================================================================

    // Calls the on_update callback for the active level (call in game loop).
    ENGINE_API void call_active_update();

    // ============================================================================
    // World/Cache Management
    // ============================================================================

    // Caches the current world state into the active level.
    ENGINE_API void cache_active_level();

    // Gets an EntityList with current world entities for saving.
    ENGINE_API EntityList get_entities_for_save(const std::string& level_id);

    // Clears all open levels and the world.
    ENGINE_API void clear_all(bool preserve_preloaded_levels = true);

    // ============================================================================
    // Entity Serialization Utilities
    // ============================================================================

    // Captures all entities from the current world into an EntityList.
    ENGINE_API EntityList capture_world_entities();

    // Instantiates entities from an EntityList into the world.
    ENGINE_API void instantiate_entities(const EntityList& entities, bool clear_world = true);

    // ============================================================================
    // Internal (used by LevelTabManager)
    // ============================================================================

    // Sets the current level path directly (for compatibility).
    ENGINE_API void set_current_level_path(const std::string& path);

    // ============================================================================
    // Scene Transition Helpers
    // ============================================================================

    // Request that the next scene (typically GameplayState) opens this level.
    // This is useful because SceneManager::load_scene() clears all level state
    // before calling Scene::load().
    ENGINE_API void set_pending_start_level(const std::string& filepath, bool append_resource_path = true);

    // Returns the pending start level filepath (or empty) and clears it.
    ENGINE_API std::string consume_pending_start_level(bool* out_append_resource_path = nullptr);

  } // namespace LevelManager

} // namespace Engine