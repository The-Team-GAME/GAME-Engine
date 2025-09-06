#include "ObjectFactory.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "BasicComponents.h"

using json = nlohmann::json;

Engine::ObjectFactory::ObjectFactory(ECS::World* world)
{
  m_world = world;
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

  if (data.contains("sprite")) {
    entity.addComponent<Components::Renderable>(
      data["sprite"]["sprite"],
      data["sprite"]["spriteType"],
      data["sprite"]["width"],
      data["sprite"]["height"]
    );
  }
}

ECS::Entity& Engine::ObjectFactory::createObject(int id)
{
  // TODO: insert return statement here
}
