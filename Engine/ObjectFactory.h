#pragma once

#include <vector>
#include "ECS/ECS.h"
#include "BasicComponents.h"
#include <optional>

namespace
{
  struct ObjectPrefab
  {
    Components::Position position;
    Components::Velocity velocity;
    Components::Size size;
    Components::Rotation rotation;
    std::optional<Components::StaticSprite> staticSprite;
  };
}

namespace Engine
{
  class ObjectFactory
  {
  private:
    ECS::World* m_world;
    std::vector<ObjectPrefab> objectList;
    int nextObjectId = 0;
  public:
    ObjectFactory(ECS::World* world);

    int fromJson(std::string jsonText);
    int fromJsonFile(std::string filePath);

    ECS::Entity createObject(int id);
  };
}