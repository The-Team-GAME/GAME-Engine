#include "Level.h"
#include "SceneManager.h"
#include "Components/Components.h"
#include "TilemapManager.h"
#include "FileIO/SettingsManager.h"
#include "FileIO/Settings.h"

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

  std::cout << "Instantiating level with " << entities.size() << " entities." << std::endl;

  Engine::ECS::for_each_entity([](Engine::ECS::Entity entity) {
    entity.destroy();
    });

  for (const auto& entity_value : entities) {
    Engine::ECS::create_entity_from_generic_value(entity_value);
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
}

std::string Engine::LevelManager::get_current_level_path()
{
  return current_level_path;
}

void Engine::LevelManager::set_current_level_path(const std::string& level_name)
{
  current_level_path = level_name;
}