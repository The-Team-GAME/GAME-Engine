//-----------------------------------------------------------------------------------
// file:    Level.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the Level class, which handles level instantiation
//          from the world and serialization of entity data.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Level.h"
#include "LevelManager.h"
#include "Components/Components.h"
#include "TilemapManager.h"
#include "FileIO/SettingsManager.h"
#include "FileIO/Settings.h"
#include "Systems/CollisionSystem.h"
#include "Logger.h"
#include "Resources.h"
#include <unordered_set>


Engine::Level Engine::Level::from_world()
{
  Level level;

  Engine::ECS::for_each_entity([&](Engine::ECS::Entity entity) {
    // Skip tile collision entities - they are temporary and rebuilt by TilemapManager
    if (entity.has_component<Components::TileCollider>()) {
      return;
    }

    // Skip the Level entity itself - it's created during instantiation
    if (entity.has_component<Components::Level>()) {
      return;
    }

    ReflectionLibrary::GenericValue entity_value = Engine::ECS::entity_to_generic_value(entity);
    level.entities.push_back(std::move(entity_value));
    });

  return level;
}

void Engine::Level::instantiate_into_world(bool clear_world)
{
  //Engine::SceneManager::load_scene("EmptyState"); // TODO: very hacky, just use empty scene for now, need better scene/level management

  Log::info("Instantiating level with {} entities.", entities.size());

  if (clear_world) {
    Engine::ECS::for_each_entity([](Engine::ECS::Entity entity) {
      entity.destroy();
      });
  }

  // Process housekeeping to ensure old entities (including tile colliders) are actually destroyed
  // before creating new ones. This prevents tile colliders from previous levels from persisting.
  Engine::ECS::process_housekeeping();

  // Create the Level entity
  Engine::ECS::Entity level_entity = Engine::ECS::create_entity();
  level_entity.add_component<Components::Level>(Engine::Resources::get_relative_path(LevelManager::get_current_level_path()));
  level_entity.add_component<Components::Transform2D>(); // Level needs transform to be a parent

  // Build old ID -> new ID remap table as we create entities
  std::unordered_map<Engine::ECS::EntityID, Engine::ECS::EntityID> id_remap;

  for (const auto& entity_value : entities) {
    // Get the original ID if it exists
    Engine::ECS::EntityID old_id = Engine::ECS::INVALID_ENTITY;
    if (entity_value.has("id") && entity_value["id"].isInt()) {
      old_id = static_cast<Engine::ECS::EntityID>(entity_value["id"].getInt());
    }

    Engine::ECS::Entity new_entity = Engine::ECS::create_entity_from_generic_value(entity_value);

    // Store the mapping from old ID to new ID
    if (old_id != Engine::ECS::INVALID_ENTITY && new_entity.is_valid()) {
      id_remap[old_id] = new_entity.get_id();
    }

    // If the entity doesn't have a parent, make it a child of the Level entity
    if (new_entity.is_valid() && !new_entity.has_component<Components::Parent>()) {
      new_entity.add_component<Components::Parent>(level_entity.get_id());
    }

    // This creates all entities including TileMap entities which store the tile data
  }

  // Fix up Parent references using the remap table
  for (auto& entity : Engine::ECS::get_entities_with<Components::Parent>()) {
    auto* parent_comp = entity.get_component<Components::Parent>();
    if (parent_comp && parent_comp->parent_id != Engine::ECS::INVALID_ENTITY) {
      auto it = id_remap.find(parent_comp->parent_id);
      if (it != id_remap.end()) {
        parent_comp->parent_id = it->second;
      }
      // If old ID not found in remap, the parent might be the Level entity (already correct)
      // or the reference is invalid
    }
  }

  // Fix up Children references using the remap table
  for (auto& entity : Engine::ECS::get_entities_with<Components::Children>()) {
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
  for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
    auto layers = entity.get_layers();
    for (Engine::ECS::LayerID layer_id = 0; layer_id < Engine::ECS::MAX_LAYERS; ++layer_id) {
      if (layers.test(layer_id)) {
        std::string layer_name = Engine::ECS::get_layer_name(layer_id);
        if (!layer_name.empty()) {
          TilemapManager::rebuild_tilemap_for_layer(layer_name);
        }
      }
    }
  }

  // Update all prefab instances to sync with latest prefab data
  // This ensures that if a prefab was modified after the level was saved,
  // instances will receive those updates while preserving overrides
  std::unordered_set<std::string> updated_prefabs;
  for (const auto& [entity_id, instance_data] : Engine::ECS::Internal::get_containers().prefab_instances) {
    // Only update each prefab once
    if (updated_prefabs.insert(instance_data.prefab_id).second) {
      Engine::ECS::update_prefab_instances(instance_data.prefab_id);
    }
  }

  // Update collision world bounds after level load
  TilemapManager::broadcast_world_bounds();
}

void Engine::Level::call_init() const
{
  if (!on_init.empty()) {
    auto func = on_init.get<LevelCallback>();
    if (func) {
      func();
    }
    else {
      Log::warning("Level on_init callback '{}' not found in FunctionRegistry", on_init.name);
    }
  }
}

void Engine::Level::call_update() const
{
  if (!on_update.empty()) {
    auto func = on_update.get<LevelCallback>();
    if (func) {
      func();
    }
    else {
      Log::warning("Level on_update callback '{}' not found in FunctionRegistry", on_update.name);
    }
  }
}

void Engine::Level::call_exit() const
{
  if (!on_exit.empty()) {
    auto func = on_exit.get<LevelCallback>();
    if (func) {
      func();
    }
    else {
      Log::warning("Level on_exit callback '{}' not found in FunctionRegistry", on_exit.name);
    }
  }
}