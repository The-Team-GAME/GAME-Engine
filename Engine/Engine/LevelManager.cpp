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
}

namespace Engine {
  namespace LevelManager {

    void reset_level_world(const std::string& level_id, LevelState& state)
    {
      if (state.cached_entities.empty()) {
        Log::warning("[LevelManager] Cannot reset level with no cached snapshot: {}", level_id);
        return;
      }

      ECS::WorldID previously_active_world = ECS::get_active_world();
      ECS::WorldID old_world = state.world_id;
      ECS::WorldID new_world = ECS::create_world();

      ECS::set_active_world(new_world);
      instantiate_entities(state.cached_entities, true);

      auto level_entity = ECS::get_first_entity_with<Components::Level>();
      if (level_entity.is_valid()) {
        auto* level_comp = level_entity.get_component<Components::Level>();
        if (level_comp) {
          level_comp->level_path = level_id;
        }
      }

      state.world_id = new_world;

      if (previously_active_world != old_world && previously_active_world != ECS::INVALID_WORLD) {
        ECS::set_active_world(previously_active_world);
      }

      if (old_world != ECS::INVALID_WORLD && old_world != ECS::get_active_world()) {
        ECS::destroy_world(old_world);
      }

      Log::info("[LevelManager] Reset level world to initial state: {}", level_id);
    }

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
      std::unordered_map<std::string, EntityList> ready;
      {
        std::lock_guard<std::mutex> lock(preload_mutex);
        if (preload_results.empty()) {
          return;
        }
        ready.swap(preload_results);
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

    EntityList get_preloaded_or_disk(LevelState& state, const std::string& level_id)
    {
      if (state.preload_ready && !state.preloaded_entities.empty()) {
        return state.preloaded_entities;
      }

      return Resources::load_level(level_id, false, false);
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
      }

      // Load the level from disk
      EntityList entities = Resources::load_level(filepath, append_resource_path, false);

      // Multi-world: create a new world, load into it
      ECS::WorldID prev_world = ECS::get_active_world();
      ECS::WorldID new_world = ECS::create_world();
      ECS::set_active_world(new_world);

      LevelState new_state;
      new_state.world_id = new_world;
      new_state.cached_entities = entities;
      new_state.preloaded_entities = entities;
      new_state.preload_ready = true;
      new_state.keep_loaded = false;

      open_levels[full_path] = std::move(new_state);
      level_order.push_back(full_path);
      active_level_id = full_path;
      current_level_path = full_path;

      // Instantiate into the new world (clear_world=true since it's fresh)
      instantiate_entities(entities, true);

      // Destroy the previous world if it's not a preloaded level
      if (prev_world != ECS::INVALID_WORLD && prev_world != new_world) {
        bool prev_is_kept = false;
        for (auto& [id, state] : open_levels) {
          if (state.world_id == prev_world && state.keep_loaded) {
            prev_is_kept = true;
            break;
          }
        }
        if (!prev_is_kept) {
          ECS::destroy_world(prev_world);
        }
      }

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

      // Multi-world: load into a separate world, then switch back
      ECS::WorldID prev_world = ECS::get_active_world();
      ECS::WorldID preload_world = ECS::create_world();
      ECS::set_active_world(preload_world);

      // Instantiate entities into the new world
      instantiate_entities(entities, true);

      LevelState new_state;
      new_state.world_id = preload_world;
      new_state.cached_entities = entities;
      new_state.preloaded_entities = entities;
      new_state.preload_ready = true;
      new_state.keep_loaded = true;

      // Switch back to the original world
      ECS::set_active_world(prev_world);

      open_levels[full_path] = std::move(new_state);
      level_order.push_back(full_path);

      Log::info("[LevelManager] Preloaded level into world {}: {}", preload_world, full_path);
      queue_preload_refresh(full_path);
      return full_path;
    }

    std::string create_new_level()
    {
      // Call on_exit for current level before creating new one
      if (!active_level_id.empty() && is_level_open(active_level_id)) {
        call_level_exit();
      }

      // Find the next available number
      size_t next_num = find_next_new_level_number();
      std::string level_id = "__new_" + std::to_string(next_num);

      ECS::WorldID new_world = ECS::create_world();
      ECS::set_active_world(new_world);

      // Create a new empty level state
      LevelState new_state;
      new_state.world_id = new_world;
      new_state.cached_entities = EntityList();
      new_state.keep_loaded = false;

      open_levels[level_id] = std::move(new_state);
      level_order.push_back(level_id);
      active_level_id = level_id;
      current_level_path = "";  // No file path for new levels

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
      }

      // Switch to new level
      active_level_id = level_id;
      current_level_path = level_id.starts_with("__new_") ? "" : level_id;

      LevelState& state = open_levels[level_id];

      // Multi-world: rebuild the target world from its pristine snapshot, then activate it
      reset_level_world(level_id, state);
      if (state.world_id != ECS::INVALID_WORLD) {
        ECS::set_active_world(state.world_id);
        Log::info("Switched to level (world {}): {}", state.world_id, level_id);
      }

      // Call on_init (via Level component if present)
      call_level_init();
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
      EntityList reload_entities = get_preloaded_or_disk(state, level_id);
      if (reload_entities.empty()) {
        Log::warning("Reload failed - no entities loaded: {}", level_id);
        return;
      }

      state.cached_entities = reload_entities;
      state.preloaded_entities = reload_entities;
      state.preload_ready = true;

      const bool is_active = (level_id == active_level_id);
      if (is_active) {
        call_level_exit();
      }

      reset_level_world(level_id, state);
      if (is_active && state.world_id != ECS::INVALID_WORLD) {
        ECS::set_active_world(state.world_id);
        current_level_path = level_id;
        call_level_init();
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

      // Call on_exit if closing active level
      if (level_id == active_level_id) {
        call_level_exit();
      }

      // Find position in order
      auto it = std::find(level_order.begin(), level_order.end(), level_id);
      size_t index = std::distance(level_order.begin(), it);

      // Multi-world: destroy the world if it's not the active one
      LevelState& closing_state = open_levels[level_id];
      ECS::WorldID closing_world = closing_state.world_id;

      if (level_id == active_level_id) {
        if (level_order.size() > 1) {
          size_t new_index = (index > 0) ? index - 1 : 1;
          std::string new_active = level_order[new_index];

          active_level_id = new_active;
          current_level_path = new_active.starts_with("__new_") ? "" : new_active;

          LevelState& state = open_levels[new_active];
          reset_level_world(new_active, state);
          if (state.world_id != ECS::INVALID_WORLD) {
            ECS::set_active_world(state.world_id);
          }
          // The old world is no longer active, safe to destroy
          ECS::destroy_world(closing_world);

          call_level_init();
        }
        else {
          // Closing last level — clear world entities
          ECS::for_each_entity([](ECS::Entity entity) {
            entity.destroy();
            });
          ECS::process_housekeeping();
          active_level_id = "";
          current_level_path = "";
        }
      }
      else {
        // Non-active level: just destroy its world
        ECS::destroy_world(closing_world);
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

      EntityList entities_to_save = get_entities_for_save(level_id);

      if (Resources::save_level(level_id, entities_to_save, false)) {
        Log::info("Saved level: {}", level_id);
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

      LevelState& state = it->second;
      state.cached_entities = capture_world_entities();
      state.preloaded_entities = state.cached_entities;
      state.preload_ready = !state.preloaded_entities.empty();
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

      // Multi-world: destroy non-preserved worlds
      ECS::WorldID active_world = ECS::get_active_world();
      std::unordered_map<std::string, LevelState> preserved_levels;
      std::vector<std::string> preserved_order;

      for (auto& [id, state] : open_levels) {
        const bool preserve_state = preserve_preloaded_levels && state.keep_loaded;

        if (preserve_state) {
          Log::info("[LevelManager] Preserving preloaded world {} across scene clear: {}", state.world_id, id);
          preserved_order.push_back(id);
          preserved_levels.emplace(id, std::move(state));
        }
        else if (state.world_id != active_world) {
          // Destroy non-active worlds directly
          ECS::destroy_world(state.world_id);
        }
      }

      open_levels = std::move(preserved_levels);
      level_order = std::move(preserved_order);
      active_level_id = "";
      current_level_path = "";

      // Destroy active entities in the current world
      ECS::for_each_entity([](ECS::Entity entity) {
        entity.destroy();
        });
      ECS::process_housekeeping();

      Log::info("Cleared levels. Preserved {} preloaded levels.", open_levels.size());
    }

    void set_current_level_path(const std::string& path)
    {
      current_level_path = path;
    }

    EntityList capture_world_entities()
    {
      EntityList entities;

      ECS::for_each_entity([&](ECS::Entity entity) {
        // Skip tile collision entities - they are temporary and rebuilt by TilemapManager
        if (entity.has_component<Components::TileCollider>()) {
          return;
        }

        ReflectionLibrary::GenericValue entity_value = ECS::entity_to_generic_value(entity);
        entities.push_back(std::move(entity_value));
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
                  font_size = data["font_size"].getInt();
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
        ECS::for_each_entity([](ECS::Entity entity) {
          entity.destroy();
          });
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
