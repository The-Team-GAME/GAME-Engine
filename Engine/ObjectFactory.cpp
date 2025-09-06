#include "ObjectFactory.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "BasicComponents.h"
#include "ECS/World.h"

using json = nlohmann::json;

Engine::ObjectFactory::ObjectFactory(ECS::World* world) : m_world(world)
{
}

int Engine::ObjectFactory::fromJson(std::string jsonText)
{
  return 0;
}

int Engine::ObjectFactory::fromJsonFile(std::string filePath)
{
  std::ifstream f(filePath);
  json data = json::parse(f);

  auto entity = m_world->createEntity();

  // We just want a prefab entity
  entity.addComponent<Components::Inactive>();


  if (data.contains("position")) {
    entity.addComponent<Components::Position>(data["position"]["x"], data["position"]["y"]);
  }

  if (data.contains("velocity")) {
    entity.addComponent<Components::Velocity>(data["velocity"]["x"], data["velocity"]["y"]);
  }

  if (data.contains("size")) {
    entity.addComponent<Components::Size>(data["size"]["width"], data["size"]["height"]);
  }

  if (data.contains("rotation")) {
    entity.addComponent<Components::Rotation>(data["rotation"]);
  }

  if (data.contains("staticSprite")) {
    if (data["staticSprite"].contains("minifyingFilter")) {
      std::string filterStr = data["staticSprite"]["minifyingFilter"];

      Graphics::Filter filter = Graphics::Filter::Linear;
      if (filterStr == "Nearest") {
        filter = Graphics::Filter::Nearest;
      }

      entity.addComponent<Components::StaticSprite>(data["staticSprite"]["texturePath"], filter);
    }
    else {
      entity.addComponent<Components::StaticSprite>(data["staticSprite"]["texturePath"]);
    }
  }

  return entity.getId();
}

ECS::Entity Engine::ObjectFactory::createObject(int id)
{
  auto newEntity = m_world->cloneEntity(id);
  newEntity.removeComponent<Components::Inactive>();
  return newEntity;
}
