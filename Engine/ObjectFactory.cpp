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

  ObjectPrefab newObject;

  if (data.contains("position")) {
    newObject.position = Components::Position(data["position"]["x"], data["position"]["y"]);
  }

  if (data.contains("velocity")) {
    newObject.velocity = Components::Velocity(data["velocity"]["x"], data["velocity"]["y"]);
  }

  if (data.contains("size")) {
    newObject.size = Components::Size(data["size"]["width"], data["size"]["height"]);
  }

  if (data.contains("rotation")) {
    newObject.rotation = Components::Rotation(data["rotation"]);
  }

  if (data.contains("staticSprite")) {
    if (data["staticSprite"].contains("minifyingFilter")) {
      std::string filterStr = data["staticSprite"]["minifyingFilter"];

      Graphics::MinifyingFilter filter = Graphics::MinifyingFilter::Linear;
      if (filterStr == "Nearest") {
        filter = Graphics::MinifyingFilter::Nearest;
      }

      newObject.staticSprite = Components::StaticSprite(data["staticSprite"]["texturePath"], filter);
    }
    else {
      newObject.staticSprite = Components::StaticSprite(data["staticSprite"]["texturePath"]);
    }
  }

  objectList.push_back(newObject);
  int id = nextObjectId;
  nextObjectId++;

  return id;
}

ECS::Entity Engine::ObjectFactory::createObject(int id)
{
  ObjectPrefab& objectPrefab = objectList[id];

  ECS::Entity entity = m_world->createEntity()
    .addComponent<Components::Position>(objectPrefab.position)
    .addComponent<Components::Velocity>(objectPrefab.velocity)
    .addComponent<Components::Rotation>(objectPrefab.rotation)
    .addComponent<Components::Size>(objectPrefab.size);

  if (objectPrefab.staticSprite.has_value()) {
    entity.addComponent<Components::StaticSprite>(objectPrefab.staticSprite.value());
  }

  return entity;
}
