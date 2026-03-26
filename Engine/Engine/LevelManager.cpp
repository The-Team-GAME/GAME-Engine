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
#include "Components/LevelComponent.h"
#include "Components/Components.h"
#include "Components/HierarchyComponents.h"
#include "TilemapManager.h"
#include <algorithm>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <chrono>

namespace {
  std::unordered_map<std::string, Engine::LevelState> open_levels;
  std::vector<std::string> level_order;  // Maintains order of open levels
  std::string active_level_id;
  std::string current_level_path;  // Actual file path (empty for new levels)
  const std::string empty_string;
  std::mutex preload_mutex;
  std::unordered_map<std::string, Engine::EntityList> preload_results;

  // Global preloaded snapshot map: level_path → deactivated entity IDs (pristine state)
  std::unordered_map<std::string, std::vector<Engine::ECS::EntityID>> preloaded_snapshots;

  // Incremental snapshot rebuild tracking
  struct SnapshotRebuild {
    std::string level_id;
    std::vector<Engine::ECS::EntityID> source_ids;  // Entity IDs to clone from
    std::vector<Engine::ECS::EntityID> built_ids;   // Cloned entity IDs so far
    std::unordered_map<Engine::ECS::EntityID, Engine::ECS::EntityID> id_remap;
    size_t index = 0;                                // Current progress
    bool active = false;
  };
  SnapshotRebuild snapshot_rebuild;

  // Helper function to check if a serialized entity is a tile collision entity
  // These are temporary entities rebuilt by TilemapManager and should not be loaded from files
  bool is_tile_collision_entity(const ReflectionLibrary::GenericValue& entity_value) {
    if (!entity_value.isObject() || !entity_value.has("components")) {
      return false;
    }
    const auto& components = entity_value["components"];
    if (!components.isArray()) {
      return false;
    }

    bool has_tile_collider = false;
    bool has_box_collider = false;
    bool has_transform2d = false;
    bool has_rigidbody = false;
    bool is_static_rigidbody = false;
    bool has_other_components = false;

    for (const auto& comp : components.getArray()) {
      if (!comp.isObject() || !comp.has("type")) continue;

      const auto& type_val = comp["type"];
      if (!type_val.isString()) continue;

      std::string type_name = type_val.getString();

      if (type_name == "Components::TileCollider") {
        has_tile_collider = true;
      }
      else if (type_name == "Components::BoxCollider") {
        has_box_collider = true;
      }
      else if (type_name == "Components::Transform2D") {
        has_transform2d = true;
      }
      else if (type_name == "Components::RigidBody") {
        has_rigidbody = true;
        if (comp.has("data") && comp["data"].isObject() && comp["data"].has("is_static")) {
          const auto& is_static = comp["data"]["is_static"];
          if (is_static.isBool() && is_static.getBool()) {
            is_static_rigidbody = true;
          }
        }
      }
      else if (type_name == "Components::Parent") {
        // Don't count Parent as "other" - legacy tile colliders have broken Parent
      }
      else {
        has_other_components = true;
      }
    }

    // Has TileCollider marker component (current format)
    if (has_tile_collider) {
      return true;
    }

    // Legacy format - has BoxCollider, Transform2D, RigidBody (static), and NO other components
    if (has_box_collider && has_transform2d && has_rigidbody && is_static_rigidbody && !has_other_components) {
      return true;
    }

    // Legacy format 2 - has BoxCollider, Transform2D, RigidBody (static), and NO other components
    if (has_transform2d && has_rigidbody && is_static_rigidbody && !has_other_components) {
      return true;
    }

    return false;
  }

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

  std::string normalize_level_path(const std::string& filepath, bool append_resource_path)
  {
    return append_resource_path
      ? Engine::Path(Engine::Resources::resource_path).join(filepath).normalize().to_string()
      : Engine::Path(filepath).normalize().to_string();
  }

  // Call on_exit via the Level component in the world (if exists)
  void call_level_exit() {
    auto level_entity = Engine::ECS::get_first_entity_with<Components::Level>();
    if (level_entity.is_valid()) {
      auto* level_comp = level_entity.get_component<Components::Level>();
      if (level_comp) {
        level_comp->call_exit();
      }
    }
  }

  // Call on_init via the Level component in the world (if exists)
  void call_level_init() {
    auto level_entity = Engine::ECS::get_first_entity_with<Components::Level>();
    if (level_entity.is_valid()) {
      auto* level_comp = level_entity.get_component<Components::Level>();
      if (level_comp) {
        level_comp->call_init();
      }
    }
  }

  bool is_reloadable_level_id(const std::string& level_id)
  {
    return !level_id.empty() && !level_id.starts_with("__new_");
  }

  void remap_cloned_hierarchy_ids(
    const std::unordered_map<Engine::ECS::EntityID, Engine::ECS::EntityID>& id_remap,
    const std::vector<Engine::ECS::EntityID>& cloned_ids)
  {
    for (Engine::ECS::EntityID clone_id : cloned_ids) {
      Engine::ECS::Entity entity(clone_id);
      if (!entity.is_valid()) {
        continue;
      }

      if (auto* parent_comp = entity.get_component<Components::Parent>()) {
        auto it = id_remap.find(parent_comp->parent_id);
        if (it != id_remap.end()) {
          parent_comp->parent_id = it->second;
        }
      }

      if (auto* children_comp = entity.get_component<Components::Children>()) {
        for (auto& child_id : children_comp->child_ids) {
          auto it = id_remap.find(child_id);
          if (it != id_remap.end()) {
            child_id = it->second;
          }
        }
      }
    }
  }

  std::vector<Engine::ECS::EntityID> clone_snapshot_entities(
    const std::vector<Engine::ECS::EntityID>& source_ids)
  {
    std::vector<Engine::ECS::EntityID> cloned_ids;
    cloned_ids.reserve(source_ids.size());

    std::unordered_map<Engine::ECS::EntityID, Engine::ECS::EntityID> id_remap;
    id_remap.reserve(source_ids.size());

    for (Engine::ECS::EntityID source_id : source_ids) {
      Engine::ECS::Entity source(source_id);
      if (!source.is_valid()) {
        continue;
      }

      Engine::ECS::Entity cloned = Engine::ECS::clone_entity(source);
      if (!cloned.is_valid()) {
        continue;
      }

      Engine::ECS::EntityID clone_id = cloned.get_id();
      Engine::ECS::deactivate_entity(clone_id);
      cloned_ids.push_back(clone_id);
      id_remap[source_id] = clone_id;
    }

    remap_cloned_hierarchy_ids(id_remap, cloned_ids);
    return cloned_ids;
  }
}

namespace Engine {
  namespace LevelManager {

    namespace {
      std::string g_pending_level_to_open;
    }

    // Forward declarations
    void build_preloaded_snapshot(LevelState& state);

    void queue_preload_refresh(const std::string& level_id)
    {
      if (!is_reloadable_level_id(level_id)) {
        return;
      }

      auto it = open_levels.find(level_id);
      if (it == open_levels.end()) {
        return;
      }

      LevelState& state = it->second;
      if (state.preload_in_progress) {
        state.preload_refresh_queued = true;
        return;
      }

      state.preload_in_progress = true;
      state.preload_refresh_queued = false;

      Log::info("[LevelManager] Preload refresh scheduled: {}", level_id);

      std::string level_path = level_id;
      std::thread([level_path]() {
        Engine::EntityList entities = Engine::FileIO::deserialize_level_from_json_file(level_path);
        std::lock_guard<std::mutex> lock(preload_mutex);
        preload_results[level_path] = std::move(entities);
        }).detach();
    }

    void process_preload_results()
    {
      // --- Handle background JSON deserialization results ---
      {
        std::unordered_map<std::string, EntityList> ready;
        {
          std::lock_guard<std::mutex> lock(preload_mutex);
          if (!preload_results.empty()) {
            ready.swap(preload_results);
          }
        }

        for (auto& [level_id, entities] : ready) {
          auto it = open_levels.find(level_id);
          if (it == open_levels.end()) {
            continue;
          }

          LevelState& state = it->second;
          state.preloaded_entities = std::move(entities);
          state.preload_ready = !state.preloaded_entities.empty();
          state.preload_in_progress = false;
          state.cached_entities = state.preloaded_entities;

          Resources::cache_level(level_id, state.preloaded_entities, false, false);
          Log::info("[LevelManager] Preload refresh completed: {}", level_id);

          if (state.preload_refresh_queued) {
            state.preload_refresh_queued = false;
            queue_preload_refresh(level_id);
          }
        }
      }

      // --- Incremental snapshot rebuild (time-budgeted) ---
      // --- Incremental snapshot rebuild (time-budgeted) ---
      // After a reload consumes the snapshot, rebuild it across multiple frames
      // by cloning a batch of entities per frame within a 2ms time budget.
      if (snapshot_rebuild.active) {
        // First frame: capture source entity IDs and clean up old snapshot
        if (snapshot_rebuild.source_ids.empty() && snapshot_rebuild.index == 0) {
          clear_preloaded_snapshot(snapshot_rebuild.level_id);
          snapshot_rebuild.id_remap.clear();

          ECS::for_each_entity([&](ECS::Entity entity) {
            snapshot_rebuild.source_ids.push_back(entity.get_id());
            });

          if (snapshot_rebuild.source_ids.empty()) {
            snapshot_rebuild.active = false;
          }
        }
        else {
          // Clone a batch of entities within the time budget
          auto deadline = std::chrono::steady_clock::now() + std::chrono::microseconds(2000);

          while (snapshot_rebuild.index < snapshot_rebuild.source_ids.size()) {
            ECS::EntityID source_id = snapshot_rebuild.source_ids[snapshot_rebuild.index];
            ECS::Entity source(source_id);

            if (source.is_valid()) {
              ECS::Entity cloned = ECS::clone_entity(source);
              if (cloned.is_valid()) {
                ECS::EntityID clone_id = cloned.get_id();
                ECS::deactivate_entity(clone_id);
                snapshot_rebuild.built_ids.push_back(clone_id);
                snapshot_rebuild.id_remap[source_id] = clone_id;
              }
            }

            ++snapshot_rebuild.index;

            if (std::chrono::steady_clock::now() >= deadline) {
              break; // Budget exhausted, continue next frame
            }
          }

          // Check if rebuild is complete
          if (snapshot_rebuild.index >= snapshot_rebuild.source_ids.size()) {
            if (!snapshot_rebuild.built_ids.empty()) {
              remap_cloned_hierarchy_ids(snapshot_rebuild.id_remap, snapshot_rebuild.built_ids);
              preloaded_snapshots[snapshot_rebuild.level_id] = std::move(snapshot_rebuild.built_ids);
            }
            snapshot_rebuild.active = false;
            snapshot_rebuild.source_ids.clear();
            snapshot_rebuild.built_ids.clear();
            snapshot_rebuild.id_remap.clear();
            snapshot_rebuild.index = 0;
            Log::info("[LevelManager] Incremental snapshot rebuild completed: {}", snapshot_rebuild.level_id);
          }
        }
      }
    }

    EntityList get_preloaded_or_disk(LevelState& state, const std::string& level_id)
    {
      if (state.preload_ready && !state.preloaded_entities.empty()) {
        return state.preloaded_entities;
      }

      return Resources::load_level(level_id, false, false);
    }

    // Reactivates a level's deactivated entities. This is the fast path — no deserialization.
    void activate_level(LevelState& state)
    {
      for (ECS::EntityID id : state.live_entity_ids) {
        ECS::activate_entity(id);
      }
      state.live_entity_ids.clear();
      state.has_live_entities = false;
    }

    // Destroys all deactivated entities owned by a level state.
    void destroy_level_entities(LevelState& state)
    {
      if (state.has_live_entities) {
        for (ECS::EntityID id : state.live_entity_ids) {
          // Re-activate so process_housekeeping can destroy them properly
          ECS::activate_entity(id);
          ECS::Entity(id).destroy();
        }
        state.live_entity_ids.clear();
        state.has_live_entities = false;
        ECS::process_housekeeping();
      }
    }

    // Deactivates all current world entities and stores their IDs in the level state.
    // This is the fast path — no serialization needed.
    void deactivate_active_level()
    {
      if (active_level_id.empty()) return;

      auto it = open_levels.find(active_level_id);
      if (it == open_levels.end()) return;

      LevelState& state = it->second;
      state.live_entity_ids.clear();

      ECS::for_each_entity([&](ECS::Entity entity) {
        state.live_entity_ids.push_back(entity.get_id());
        });

      // Deactivate all entities (removes from active set and layer sets, preserves data)
      for (ECS::EntityID id : state.live_entity_ids) {
        ECS::deactivate_entity(id);
      }

      state.has_live_entities = true;
      state.cached_entities.clear(); // No serialized cache needed

      Log::info("Deactivated level: {} ({} entities)",
        active_level_id,
        state.live_entity_ids.size());
    }

    bool has_preloaded_snapshot(const std::string& level_id)
    {
      return preloaded_snapshots.find(level_id) != preloaded_snapshots.end() && !preloaded_snapshots[level_id].empty();
    }

    void clear_preloaded_snapshot(const std::string& level_id)
    {
      auto it = preloaded_snapshots.find(level_id);
      if (it != preloaded_snapshots.end()) {
        for (ECS::EntityID id : it->second) {
          if (ECS::get_entity(id).is_valid()) {
            ECS::activate_entity(id);
            ECS::Entity(id).destroy();
          }
        }
        it->second.clear();
        preloaded_snapshots.erase(it);
        ECS::process_housekeeping();
      }
    }

    void build_preloaded_snapshot(LevelState& state)
    {
      (void)state;
      if (active_level_id.empty()) return;
      clear_preloaded_snapshot(active_level_id);
      std::vector<ECS::EntityID> source_ids;
      ECS::for_each_entity([&](ECS::Entity entity) {
        source_ids.push_back(entity.get_id());
        });

      auto& snapshot = preloaded_snapshots[active_level_id];
      snapshot = clone_snapshot_entities(source_ids);

      Log::info("[LevelManager] Built preloaded snapshot with {} entities.", snapshot.size());
    }

    std::string open_level(const std::string& filepath, bool append_resource_path)
    {
      // Get the full normalized path - this becomes the level ID
      std::string full_path = normalize_level_path(filepath, append_resource_path);

      // Check if this level is already open
      if (is_level_open(full_path)) {
        if (full_path == active_level_id) {
          reload_level(full_path);
          Log::info("Reloaded active level: {}", full_path);
          return full_path;
        }

        switch_to_level(full_path);
        Log::info("Switched to existing level: {}", full_path);
        return full_path;
      }

      // Call on_exit for current level before switching
      if (!active_level_id.empty() && is_level_open(active_level_id)) {
        call_level_exit();
        cache_active_level();
      }

      // Load the level from disk
      EntityList entities = Resources::load_level(filepath, append_resource_path, false);

      // Single-world approach
      LevelState new_state;
      new_state.cached_entities = EntityList();
      new_state.preloaded_entities = entities;
      new_state.preload_ready = true;
      new_state.keep_loaded = false;

      open_levels[full_path] = std::move(new_state);
      level_order.push_back(full_path);
      active_level_id = full_path;
      current_level_path = full_path;

      // Instantiate into world
      instantiate_entities(entities, true);

      // Set the level path on the Level component
      auto level_entity = ECS::get_first_entity_with<Components::Level>();
      if (level_entity.is_valid()) {
        auto* level_comp = level_entity.get_component<Components::Level>();
        if (level_comp) {
          level_comp->level_path = filepath;
        }
      }

      // Call on_init callback (via Level component if present)
      call_level_init();

      build_preloaded_snapshot(open_levels[full_path]);

      queue_preload_refresh(full_path);

      Log::info("Opened level: {}", full_path);
      EventManager::send_event<Events::ToastEvent>(
        "Opened: " + Path(full_path).get_filename(),
        Events::ToastType::Info,
        2.0f
      );

      return full_path;
    }

    std::string preload_level(const std::string& filepath, bool append_resource_path)
    {
      std::string full_path = normalize_level_path(filepath, append_resource_path);

      Log::info("[LevelManager] Preload requested: {}", full_path);

      auto existing = open_levels.find(full_path);
      if (existing != open_levels.end()) {
        existing->second.keep_loaded = true;
        queue_preload_refresh(full_path);
        Log::info("[LevelManager] Level already resident, marking keep_loaded: {}", full_path);
        return full_path;
      }

      EntityList entities = Resources::load_level(filepath, append_resource_path, false);

      // Single-world approach with deactivation
      std::unordered_set<ECS::EntityID> existing_ids;
      ECS::for_each_entity([&](ECS::Entity entity) {
        existing_ids.insert(entity.get_id());
        });

      instantiate_entities(entities, false);

      LevelState new_state;
      ECS::for_each_entity([&](ECS::Entity entity) {
        if (existing_ids.find(entity.get_id()) == existing_ids.end()) {
          new_state.live_entity_ids.push_back(entity.get_id());
        }
        });

      for (ECS::EntityID id : new_state.live_entity_ids) {
        ECS::deactivate_entity(id);
      }

      // Build snapshot for instant reload
      clear_preloaded_snapshot(full_path);
      auto& snapshot = preloaded_snapshots[full_path];
      snapshot = clone_snapshot_entities(new_state.live_entity_ids);

      new_state.has_live_entities = !new_state.live_entity_ids.empty();
      new_state.keep_loaded = true;
      new_state.preloaded_entities = entities;
      new_state.preload_ready = true;

      open_levels[full_path] = std::move(new_state);
      level_order.push_back(full_path);

      Log::info("[LevelManager] Preloaded level as inactive: {} ({} entities)", full_path, open_levels[full_path].live_entity_ids.size());
      queue_preload_refresh(full_path);
      return full_path;
    }

    std::string create_new_level()
    {
      // Call on_exit for current level before creating new one
      if (!active_level_id.empty() && is_level_open(active_level_id)) {
        call_level_exit();
        cache_active_level();
      }

      // Find the next available number
      size_t next_num = find_next_new_level_number();
      std::string level_id = "__new_" + std::to_string(next_num);

      // Create a new empty level state
      LevelState new_state;
      new_state.cached_entities = EntityList();
      new_state.keep_loaded = false;

      open_levels[level_id] = std::move(new_state);
      level_order.push_back(level_id);
      active_level_id = level_id;
      current_level_path = "";  // No file path for new levels

      // Clear the world
      // IMPORTANT: clear all ACTIVE entities (even ones with no layer).
      // ECS::for_each_entity() iterates by layer sets and can miss unlayered
      // entities, causing levels to accumulate.
      for (auto entity : ECS::get_all_entities()) {
        entity.destroy();
      }
      ECS::process_housekeeping();

      // Create the Level entity with Level component
      ECS::Entity level_entity = ECS::create_entity();
      level_entity.add_component<Components::Level>();

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
        call_level_exit();
        cache_active_level();
      }

      // Switch to new level
      active_level_id = level_id;
      current_level_path = level_id.starts_with("__new_") ? "" : level_id;

      LevelState& state = open_levels[level_id];

      // Single-world approach
      if (state.has_live_entities) {
        // Fast path: reactivate deactivated entities
        activate_level(state);
      }
      else if (has_preloaded_snapshot(level_id)) {
        for (ECS::EntityID id : preloaded_snapshots[level_id]) {
          ECS::activate_entity(id);
        }
        preloaded_snapshots.erase(level_id);
      }
      else {
        // Cold path: instantiate from resident preloaded data when available
        EntityList entities = get_preloaded_or_disk(state, level_id);
        instantiate_entities(entities, true);
        state.cached_entities = entities;
      }

      // Ensure tile collision entities and collision world bounds are correct after a fast switch.
      // The cold-load path handles this inside instantiate_entities().
      for (auto& entity : ECS::get_entities_with<Components::TileMap>()) {
        auto layers = entity.get_layers();
        for (ECS::LayerID layer_id = 0; layer_id < ECS::MAX_LAYERS; ++layer_id) {
          if (layers.test(layer_id)) {
            std::string layer_name = ECS::get_layer_name(layer_id);
            if (!layer_name.empty()) {
              TilemapManager::rebuild_tilemap_for_layer(layer_name);
            }
          }
        }
      }

      TilemapManager::broadcast_world_bounds();

      Log::info("Switched to level: {}", level_id);

      // Call on_init (via Level component if present)
      call_level_init();

      if (!has_preloaded_snapshot(level_id)) {
        build_preloaded_snapshot(state);
      }
    }

    void reload_level(const std::string& level_id)
    {
      if (!is_level_open(level_id)) {
        Log::warning("Cannot reload level - not open: {}", level_id);
        return;
      }

      if (!is_reloadable_level_id(level_id)) {
        Log::warning("Cannot reload level without a file path: {}", level_id);
        return;
      }

      auto it = open_levels.find(level_id);
      if (it == open_levels.end()) {
        return;
      }

      LevelState& state = it->second;
      const bool is_active = (level_id == active_level_id);

      // Cancel any in-progress incremental rebuild for this level
      if (snapshot_rebuild.active && snapshot_rebuild.level_id == level_id) {
        // Destroy any partially-cloned snapshot entities
        for (auto id : snapshot_rebuild.built_ids) {
          if (ECS::get_entity(id).is_valid()) {
            ECS::activate_entity(id);
            ECS::Entity(id).destroy();
          }
        }
        snapshot_rebuild.active = false;
        snapshot_rebuild.source_ids.clear();
        snapshot_rebuild.built_ids.clear();
        snapshot_rebuild.id_remap.clear();
        snapshot_rebuild.index = 0;
        ECS::process_housekeeping();
      }

      if (has_preloaded_snapshot(level_id) && is_active) {
        if (is_active) {
          call_level_exit();
        }

        for (auto entity : ECS::get_all_entities()) {
          entity.destroy();
        }
        ECS::process_housekeeping();

        for (ECS::EntityID id : preloaded_snapshots[level_id]) {
          ECS::activate_entity(id);
        }
        preloaded_snapshots.erase(level_id);

        // After reactivating entities from the snapshot, rebuild tile collision entities
        // and world bounds. This matches the normal level-switch path and prevents
        // "blank" reloads where TilemapManager state wasn't rebuilt.
        for (auto& entity : ECS::get_entities_with<Components::TileMap>()) {
          auto layers = entity.get_layers();
          for (ECS::LayerID layer_id = 0; layer_id < ECS::MAX_LAYERS; ++layer_id) {
            if (layers.test(layer_id)) {
              std::string layer_name = ECS::get_layer_name(layer_id);
              if (!layer_name.empty()) {
                TilemapManager::rebuild_tilemap_for_layer(layer_name);
              }
            }
          }
        }
        TilemapManager::broadcast_world_bounds();

        current_level_path = level_id;
        call_level_init();

        // Defer snapshot rebuild to next frame so reload returns instantly
        snapshot_rebuild.active = true;
        snapshot_rebuild.level_id = level_id;
        snapshot_rebuild.source_ids.clear();
        snapshot_rebuild.built_ids.clear();
        snapshot_rebuild.id_remap.clear();
        snapshot_rebuild.index = 0;

        queue_preload_refresh(level_id);
        Log::info("[LevelManager] Reloaded level from preloaded snapshot: {}", level_id);
        return;
      }

      EntityList reload_entities = get_preloaded_or_disk(state, level_id);
      if (reload_entities.empty()) {
        Log::warning("Reload failed - no entities loaded: {}", level_id);
        return;
      }

      state.cached_entities = reload_entities;
      state.preloaded_entities = reload_entities;
      state.preload_ready = true;

      if (is_active) {
        call_level_exit();
      }

      if (is_active) {
        instantiate_entities(reload_entities, true);
        current_level_path = level_id;
        call_level_init();
        // Defer snapshot rebuild incrementally so reload returns instantly
        snapshot_rebuild.active = true;
        snapshot_rebuild.level_id = level_id;
        snapshot_rebuild.source_ids.clear();
        snapshot_rebuild.built_ids.clear();
        snapshot_rebuild.id_remap.clear();
        snapshot_rebuild.index = 0;
      }
      else {
        // If this level has stale deactivated live entities, destroy them so
        // reopening starts from pristine state and does not accumulate entities.
        destroy_level_entities(state);
        state.has_live_entities = false;
        state.live_entity_ids.clear();
      }

      queue_preload_refresh(level_id);
      Log::info("[LevelManager] Reloaded level: {}", level_id);
    }

    bool close_level(const std::string& level_id)
    {
      if (!is_level_open(level_id)) {
        Log::warning("Cannot close level - not open: {}", level_id);
        return false;
      }

      LevelState& closing_state = open_levels[level_id];

      // Call on_exit if closing active level
      if (level_id == active_level_id) {
        call_level_exit();
      }

      // Find position in order
      auto it = std::find(level_order.begin(), level_order.end(), level_id);
      size_t index = std::distance(level_order.begin(), it);

      // Single-world approach
      // If closing a non-active level with live entities, destroy them
      if (level_id != active_level_id) {
        destroy_level_entities(closing_state);
      }

      // If closing active level, switch to another
      if (level_id == active_level_id) {
        if (level_order.size() > 1) {
          size_t new_index = (index > 0) ? index - 1 : 1;
          std::string new_active = level_order[new_index];

          // Destroy active entities first
          for (auto entity : ECS::get_all_entities()) {
            entity.destroy();
          }
          ECS::process_housekeeping();

          active_level_id = new_active;
          current_level_path = new_active.starts_with("__new_") ? "" : new_active;

          LevelState& state = open_levels[new_active];
          if (state.has_live_entities) {
            activate_level(state);
          }
          else {
            instantiate_entities(state.cached_entities, true);
            state.cached_entities = EntityList();
          }

          call_level_init();
          build_preloaded_snapshot(state);
        }
        else {
          // Closing last level
          for (auto entity : ECS::get_all_entities()) {
            entity.destroy();
          }
          ECS::process_housekeeping();
          active_level_id = "";
          current_level_path = "";
        }
      }

      clear_preloaded_snapshot(level_id);

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

      LevelState& state = open_levels[level_id];

      if (level_id.starts_with("__new_")) {
        Log::warning("Cannot save new level without path. Use save_level_as instead.");
        return false;
      }

      EntityList entities_to_save = get_entities_for_save(level_id);

      if (Resources::save_level(level_id, entities_to_save, false)) {
        Log::info("Saved level: {}", level_id);
        if (level_id == active_level_id) {
          build_preloaded_snapshot(state);
        }
        queue_preload_refresh(level_id);
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

      EntityList entities_to_save = get_entities_for_save(level_id);

      if (!Resources::save_level(new_level_id, entities_to_save, false)) {
        Log::error("Failed to save level as: {}", new_level_id);
        return "";
      }

      // Move state to new ID
      LevelState old_state = std::move(open_levels[level_id]);
      old_state.preload_in_progress = false;
      old_state.preload_refresh_queued = false;

      auto it = std::find(level_order.begin(), level_order.end(), level_id);
      *it = new_level_id;

      open_levels.erase(level_id);
      open_levels[new_level_id] = std::move(old_state);

      if (active_level_id == level_id) {
        active_level_id = new_level_id;
        current_level_path = new_level_id;
      }

      Log::info("Saved level as: {}", new_level_id);
      queue_preload_refresh(new_level_id);
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
      process_preload_results();

      // Call on_update via the Level component in the world
      auto level_entity = ECS::get_first_entity_with<Components::Level>();
      if (level_entity.is_valid()) {
        auto* level_comp = level_entity.get_component<Components::Level>();
        if (level_comp) {
          level_comp->call_update();
        }
      }
    }

    void cache_active_level()
    {
      if (active_level_id.empty()) return;

      auto it = open_levels.find(active_level_id);
      if (it == open_levels.end()) return;

      // Use fast deactivation path instead of full serialization
      deactivate_active_level();
    }

    EntityList get_entities_for_save(const std::string& level_id)
    {
      auto it = open_levels.find(level_id);
      if (it == open_levels.end()) {
        return EntityList();
      }

      LevelState& state = it->second;

      // Get entities from world if active
      if (level_id == active_level_id) {
        return capture_world_entities();
      }

      // If level has live deactivated entities, temporarily activate to serialize
      if (state.has_live_entities) {
        // Activate, serialize, then deactivate again
        activate_level(state);
        EntityList result = capture_world_entities();

        // Re-deactivate them
        state.live_entity_ids.clear();
        ECS::for_each_entity([&](ECS::Entity entity) {
          state.live_entity_ids.push_back(entity.get_id());
          });
        for (ECS::EntityID id : state.live_entity_ids) {
          ECS::deactivate_entity(id);
        }
        state.has_live_entities = true;

        return result;
      }

      // Fall back to serialized cache
      if (state.preload_ready && !state.preloaded_entities.empty()) {
        return state.preloaded_entities;
      }

      return state.cached_entities;
    }

    void clear_all(bool preserve_preloaded_levels)
    {
      Log::info("[LevelManager] Clearing levels. preserve_preloaded_levels={}", preserve_preloaded_levels);

      // Call on_exit for active level
      if (!active_level_id.empty() && is_level_open(active_level_id)) {
        call_level_exit();
      }

      // Single-world approach
      if (preserve_preloaded_levels && !active_level_id.empty()) {
        auto active_it = open_levels.find(active_level_id);
        if (active_it != open_levels.end() && active_it->second.keep_loaded) {
          Log::info("[LevelManager] Active level is preloaded; preserving snapshot for scene clear: {}", active_level_id);
        }
      }

      std::unordered_map<std::string, LevelState> preserved_levels;
      std::vector<std::string> preserved_order;

      // Destroy all deactivated entities unless explicitly preserved.
      for (auto& [id, state] : open_levels) {
        const bool preserve_state = preserve_preloaded_levels && state.keep_loaded;

        if (preserve_state) {
          // Preserve only pristine preload snapshot; drop dirty live entities.
          destroy_level_entities(state);
          Log::info("[LevelManager] Preserving preloaded level across scene clear: {}", id);
          preserved_order.push_back(id);
          preserved_levels.emplace(id, std::move(state));
        }
        else if (id == active_level_id) {
          clear_preloaded_snapshot(id);
        }
        else if (id != active_level_id) {
          destroy_level_entities(state);
          clear_preloaded_snapshot(id);
        }
      }

      open_levels = std::move(preserved_levels);
      level_order = std::move(preserved_order);
      active_level_id = "";
      current_level_path = "";

      // Clear all ACTIVE entities (including unlayered). Deactivated entities
      // are managed per-level via destroy_level_entities()/activate_level().
      for (auto entity : ECS::get_all_entities()) {
        entity.destroy();
      }
      ECS::process_housekeeping();

      Log::info("Cleared levels. Preserved {} preloaded levels.", open_levels.size());
    }

    void set_current_level_path(const std::string& path)
    {
      current_level_path = path;
    }

    void set_pending_level_to_open(const std::string& level_path)
    {
      g_pending_level_to_open = level_path;
      Log::info("[LevelManager] Pending level set: {}", g_pending_level_to_open.empty() ? std::string("<empty>") : g_pending_level_to_open);
    }

    std::string consume_pending_level_to_open()
    {
      std::string out = std::move(g_pending_level_to_open);
      g_pending_level_to_open.clear();
      return out;
    }

    EntityList capture_world_entities()
    {
      EntityList entities;

      ECS::for_each_entity([&](ECS::Entity entity) {
        // Skip tile collision entities - they are temporary and rebuilt by TilemapManager
        if (entity.has_component<Components::TileCollider>()) {
          return;
        }
        // Skip runtime-only entities spawned by scene logic (e.g. UI buttons, transient zones)
        if (entity.has_component<Components::RuntimeOnly>()) {
          return;
        }

        ReflectionLibrary::GenericValue entity_value = ECS::entity_to_generic_value(entity);
        entities.push_back(std::move(entity_value));
        });

      // Sort entities by ID for deterministic JSON output
      std::sort(entities.begin(), entities.end(),
        [](const ReflectionLibrary::GenericValue& a, const ReflectionLibrary::GenericValue& b) {
          int64_t id_a = a.has("id") ? a["id"].getInt() : 0;
          int64_t id_b = b.has("id") ? b["id"].getInt() : 0;
          return id_a < id_b;
        });

      return entities;
    }

    void instantiate_entities(const EntityList& entities, bool clear_world)
    {
      Log::info("Instantiating level with {} entities.", entities.size());

      // Pre-scan all entities to warm the texture and font caches before instantiation.
      // This avoids redundant cache lookups during component deserialization and ensures
      // all unique resources are loaded once upfront.
      {
        std::unordered_set<std::string> texture_paths;
        std::unordered_set<std::string> font_keys; // "path|size" to deduplicate
        struct FontEntry { std::string path; int size; };
        std::vector<FontEntry> font_entries;

        for (const auto& entity_value : entities) {
          if (!entity_value.has("components") || !entity_value["components"].isArray()) continue;

          const auto& components = entity_value["components"].getArray();
          for (const auto& comp : components) {
            if (!comp.has("type") || !comp.has("data")) continue;

            const std::string& type = comp["type"].getString();
            const auto& data = comp["data"];

            if (type == "Components::Texture") {
              if (data.isObject() && data.has("path") && data["path"].isString()) {
                const std::string& path = data["path"].getString();
                if (!path.empty()) {
                  texture_paths.insert(path);
                }
              }
            }
            else if (type == "Components::Font") {
              if (data.isObject() && data.has("path") && data["path"].isString()) {
                const std::string& path = data["path"].getString();
                int font_size = 32; // default
                if (data.has("font_size") && data["font_size"].isInt()) {
                  font_size = static_cast<int>(data["font_size"].getInt());
                }
                if (!path.empty()) {
                  std::string key = path + "|" + std::to_string(font_size);
                  if (font_keys.insert(key).second) {
                    font_entries.push_back({ path, font_size });
                  }
                }
              }
            }
          }
        }

        // Batch-load all unique textures and fonts to warm the cache
        for (const auto& path : texture_paths) {
          Resources::load_texture(path, true);
        }
        for (const auto& entry : font_entries) {
          Resources::load_font(entry.path, entry.size, true);
        }
      }

      if (clear_world) {
        for (auto entity : ECS::get_all_entities()) {
          entity.destroy();
        }
      }

      // Process housekeeping to ensure old entities (including tile colliders) are actually destroyed
      // before creating new ones. This prevents tile colliders from previous levels from persisting.
      ECS::process_housekeeping();

      // Build old ID -> new ID remap table as we create entities
      std::unordered_map<ECS::EntityID, ECS::EntityID> id_remap;

      for (const auto& entity_value : entities) {
        // Skip tile collision entities - they are temporary and rebuilt by TilemapManager
        // This handles both current format (with TileCollider) and legacy format
        if (is_tile_collision_entity(entity_value)) {
          continue;
        }

        // Get the original ID if it exists
        ECS::EntityID old_id = ECS::INVALID_ENTITY;
        if (entity_value.has("id") && entity_value["id"].isInt()) {
          old_id = static_cast<ECS::EntityID>(entity_value["id"].getInt());
        }

        ECS::Entity new_entity = ECS::create_entity_from_generic_value(entity_value);

        // Store the mapping from old ID to new ID
        if (old_id != ECS::INVALID_ENTITY && new_entity.is_valid()) {
          id_remap[old_id] = new_entity.get_id();
        }
      }

      // Fix up Parent references using the remap table
      for (auto& entity : ECS::get_entities_with<Components::Parent>()) {
        auto* parent_comp = entity.get_component<Components::Parent>();
        if (parent_comp && parent_comp->parent_id != ECS::INVALID_ENTITY) {
          auto it = id_remap.find(parent_comp->parent_id);
          if (it != id_remap.end()) {
            parent_comp->parent_id = it->second;
          }
        }
      }

      // Fix up Children references using the remap table
      for (auto& entity : ECS::get_entities_with<Components::Children>()) {
        auto* children_comp = entity.get_component<Components::Children>();
        if (children_comp) {
          for (auto& child_id : children_comp->child_ids) {
            auto it = id_remap.find(child_id);
            if (it != id_remap.end()) {
              child_id = it->second;
            }
          }
        }
      }

      // Rebuild collision entities from all TileMap entities
      for (auto& entity : ECS::get_entities_with<Components::TileMap>()) {
        auto layers = entity.get_layers();
        for (ECS::LayerID layer_id = 0; layer_id < ECS::MAX_LAYERS; ++layer_id) {
          if (layers.test(layer_id)) {
            std::string layer_name = ECS::get_layer_name(layer_id);
            if (!layer_name.empty()) {
              TilemapManager::rebuild_tilemap_for_layer(layer_name);
            }
          }
        }
      }

      // Update all prefab instances to sync with latest prefab data
      std::unordered_set<std::string> updated_prefabs;
      for (const auto& [entity_id, instance_data] : ECS::Internal::get_containers().prefab_instances) {
        if (updated_prefabs.insert(instance_data.prefab_id).second) {
          ECS::update_prefab_instances(instance_data.prefab_id);
        }
      }

      // Update collision world bounds after level load
      TilemapManager::broadcast_world_bounds();

      // Ensure a Level entity exists (for old levels that don't have one)
      auto level_entity = ECS::get_first_entity_with<Components::Level>();
      if (!level_entity.is_valid()) {
        Log::info("Level entity not found, creating one for compatibility.");
        level_entity = ECS::create_entity();
        level_entity.add_component<Components::Level>();
      }

      // Make all root entities (entities without a parent) children of the Level entity
      ECS::EntityID level_entity_id = level_entity.get_id();
      ECS::for_each_entity([level_entity_id](ECS::Entity entity) {
        // Skip the Level entity itself
        if (entity.has_component<Components::Level>()) {
          return;
        }
        // Skip entities that already have a parent
        if (entity.has_component<Components::Parent>()) {
          return;
        }
        // Skip tile colliders (temporary entities)
        if (entity.has_component<Components::TileCollider>()) {
          return;
        }
        // Add Parent component to make this entity a child of the Level entity
        entity.add_component<Components::Parent>(level_entity_id);
        });
    }

  } // namespace LevelManager
} // namespace Engine
