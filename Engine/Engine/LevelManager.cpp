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

namespace {
  std::unordered_map<std::string, Engine::LevelState> open_levels;
  std::vector<std::string> level_order;  // Maintains order of open levels
  std::string active_level_id;
  std::string current_level_path;  // Actual file path (empty for new levels)
  const std::string empty_string;

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
        call_level_exit();
        cache_active_level();
      }

      // Load the level from disk
      EntityList entities = Resources::load_level(filepath, append_resource_path, false);

      // Create a new level state
      LevelState new_state;
      new_state.cached_entities = EntityList();  // Empty - entities go into world

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
        call_level_exit();
        cache_active_level();
      }

      // Find the next available number
      size_t next_num = find_next_new_level_number();
      std::string level_id = "__new_" + std::to_string(next_num);

      // Create a new empty level state
      LevelState new_state;
      new_state.cached_entities = EntityList();

      open_levels[level_id] = std::move(new_state);
      level_order.push_back(level_id);
      active_level_id = level_id;
      current_level_path = "";  // No file path for new levels

      // Clear the world
      ECS::for_each_entity([](ECS::Entity entity) {
        entity.destroy();
        });
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

      // Instantiate cached level into world
      instantiate_entities(state.cached_entities, true);
      state.cached_entities = EntityList();  // Clear cache

      // Call on_init (via Level component if present)
      call_level_init();

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
        call_level_exit();
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
          instantiate_entities(state.cached_entities, true);
          state.cached_entities = EntityList();
          active_level_id = new_active;

          call_level_init();
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

      EntityList entities_to_save = get_entities_for_save(level_id);

      if (Resources::save_level(level_id, entities_to_save, false)) {
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

      EntityList entities_to_save = get_entities_for_save(level_id);

      if (!Resources::save_level(new_level_id, entities_to_save, false)) {
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

      Log::info("Cached level: {} ({} entities)",
        active_level_id,
        state.cached_entities.size());
    }

    EntityList get_entities_for_save(const std::string& level_id)
    {
      auto it = open_levels.find(level_id);
      if (it == open_levels.end()) {
        return EntityList();
      }

      LevelState& state = it->second;

      // Get entities from world if active, otherwise from cache
      if (level_id == active_level_id) {
        return capture_world_entities();
      }
      else {
        return state.cached_entities;
      }
    }

    void clear_all()
    {
      // Call on_exit for active level
      if (!active_level_id.empty() && is_level_open(active_level_id)) {
        call_level_exit();
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
