#pragma once

#include <string>
#include <vector>
#include "ECS/Types.h"
#include "TypeDef.h"

namespace Engine
{
  struct EntityData
  {
    std::string prefabId;
    f32 x = 0.0f;
    f32 y = 0.0f;
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