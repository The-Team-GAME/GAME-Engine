#pragma once

#include <vector>
#include "ECS/ECS.h"

namespace Engine
{
  class ObjectFactory
  {
  private:
    ECS::World* m_world;
    std::vector<ECS::Entity> objectList;
  public:
    ObjectFactory(ECS::World* world);

    int fromJson(std::string jsonText);
    int fromJsonFile(std::string filePath);

    ECS::Entity& createObject(int id);
  };
}