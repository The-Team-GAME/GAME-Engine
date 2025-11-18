#include "Level.h"
#include "SceneManager.h"
#include "Components/Components.h"

namespace
{
  std::string current_level_path = "";
}


Engine::Level Engine::Level::from_world()
{
  Level level;

  Engine::ECS::for_each_entity([&](Engine::ECS::Entity entity) {
    // Skip tile entities - they are temporary and rebuilt by TilemapManager
    if (entity.has_components<Components::Tile>()) {
      return;
    }

    ReflectionLibrary::GenericValue entity_value = Engine::ECS::entity_to_generic_value(entity);
    level.entities.push_back(std::move(entity_value));
    });

  return level;
}

void Engine::Level::instantiate_into_world()
{
  Engine::SceneManager::load_scene("EmptyState"); // TODO: very hacky, just use empty scene for now, need better scene/level management

  Engine::ECS::for_each_entity([](Engine::ECS::Entity entity) {
    entity.destroy();
    });

  for (const auto& entity_value : entities) {
    Engine::ECS::create_entity_from_generic_value(entity_value);
    // TODO: we need a way to mark the entities as belonging to a specific level
  }

  // After loading all entities, rebuild tile entities from TileMap components
  // This is done in TilemapPanel::init() which gets called after level load
}

std::string Engine::LevelManager::get_current_level_path()
{
  return current_level_path;
}

void Engine::LevelManager::set_current_level_path(const std::string& level_name)
{
  current_level_path = level_name;
}