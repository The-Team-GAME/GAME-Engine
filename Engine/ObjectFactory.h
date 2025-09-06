#pragma once
#include <vector>
#include "ECS/ECS.h"

namespace Engine
{
  ECS::World* m_world;

  class ObjectFactory
  {
  private:
    std::vector<ECS::Entity> objectList;
  public:
    ObjectFactory(ECS::World* world);

    int fromJson(std::string jsonText);
    int fromJsonFile(std::string filePath);

    ECS::Entity& createObject(int id);
  };
}