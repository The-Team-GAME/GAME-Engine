#include "Level.h"
#include "SceneManager.h"

namespace
{
  std::string current_level_name = "";
}


Engine::Level Engine::Level::from_world(const std::string& level_name)
{
  Level level;
  level.name = level_name;

  Engine::ECS::for_each_entity([&](Engine::ECS::Entity entity) {
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

  current_level_name = name;
}

std::string Engine::LevelManager::get_current_level_name()
{
  return current_level_name;
}

void Engine::LevelManager::set_current_level_name(const std::string& level_name)
{
  current_level_name = level_name;
}