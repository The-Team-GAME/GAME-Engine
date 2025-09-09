#pragma once

#include <vector>
#include "ECS/ECS.h"
#include "BasicComponents.h"
#include <optional>

namespace
{
  struct ObjectPrefab
  {
    Components::ObjectId objectId;
    Components::Size size;
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

    int registerObject(std::string filePath);
    void unregisterObject(int id);

    ECS::Entity createObject(int id);
  };
}