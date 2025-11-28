#include "Level.h"
#include "SceneManager.h"
#include "Components/Components.h"
#include "TilemapManager.h"
#include "FileIO/SettingsManager.h"
#include "FileIO/Settings.h"
#include "Systems/CollisionSystem.h"
#include "Logger.h"
#include <unordered_set>

namespace
{
  std::string current_level_path = "";
}


Engine::Level Engine::Level::from_world()
{
  Level level;

  Engine::ECS::for_each_entity([&](Engine::ECS::Entity entity) {
    // Skip tile entities - they are temporary and rebuilt by TilemapManager
    if (entity.has_components<Components::TileCollider>()) {
      return;
    }

    ReflectionLibrary::GenericValue entity_value = Engine::ECS::entity_to_generic_value(entity);
    level.entities.push_back(std::move(entity_value));
    });

  return level;
}

void Engine::Level::instantiate_into_world()
{
  //Engine::SceneManager::load_scene("EmptyState"); // TODO: very hacky, just use empty scene for now, need better scene/level management

  Log::info("Instantiating level with {} entities.", entities.size());

  Engine::ECS::for_each_entity([](Engine::ECS::Entity entity) {
    entity.destroy();
    });

  // Process housekeeping to ensure old entities (including tile colliders) are actually destroyed
  // before creating new ones. This prevents tile colliders from previous levels from persisting.
  Engine::ECS::process_housekeeping();

  // Create the Level entity
  Engine::ECS::Entity level_entity = Engine::ECS::create_entity();
  level_entity.add_component<Components::Level>(LevelManager::get_current_level_path());
  level_entity.add_component<Components::Transform2D>(); // Level needs transform to be a parent

  for (const auto& entity_value : entities) {
    Engine::ECS::Entity new_entity = Engine::ECS::create_entity_from_generic_value(entity_value);
    
    // If the entity doesn't have a parent, make it a child of the Level entity
    if (new_entity.is_valid() && !new_entity.has_component<Components::Parent>()) {
      new_entity.add_component<Components::Parent>(level_entity.get_id());
    }
    
    // This creates all entities including TileMap entities which store the tile data
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

std::string Engine::LevelManager::get_current_level_path()
{
  return current_level_path;
}

void Engine::LevelManager::set_current_level_path(const std::string& level_name)
{
  current_level_path = level_name;
}