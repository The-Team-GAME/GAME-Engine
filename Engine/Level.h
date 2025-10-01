#pragma once

#include <string>
#include <vector>
#include "ECS/Types.h"

namespace Engine
{
  struct EntityData
  {
    std::string prefabId;
    float x = 0.0f;
    float y = 0.0f;
  };

  struct Level
  {
    std::string levelId;
    std::vector<std::string> objectPrefabPaths;
    std::vector<EntityData> entities;
    std::vector<int> loadedPrefabIds; // Track which prefabs were loaded
    std::vector<ECS::EntityID> spawnedEntities; // Track spawned entities
  };
}