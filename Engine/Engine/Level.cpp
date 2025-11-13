#include "Level.h"

Engine::Level Engine::Level::from_world()
{
  Level level;

  Engine::ECS::for_each_entity([&](Engine::ECS::Entity entity) {
    ReflectionLibrary::GenericValue entity_value = Engine::ECS::entity_to_generic_value(entity);
    level.entities.push_back(std::move(entity_value));
    });

  return level;
}

void Engine::Level::instantiate_into_world()
{
  for (const auto& entity_value : entities) {
    Engine::ECS::create_entity_from_generic_value(entity_value);
    // TODO: we need a way to mark the entities as belonging to a specific level
  }
}
