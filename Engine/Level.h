//-----------------------------------------------------------------------------------
// file:    Level.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the Level data structures used for level serialization.
//          Includes entity placement data, prefab references, and runtime tracking
//          for loaded/spawned entities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include "ECS/Types.h"
#include "TypeDef.h"

namespace Engine
{
  /* ========================== //
  // Entity placement data      //
  // ========================== */
  struct EntityData
  {
    std::string prefabId;
    f32 x = 0.0f;
    f32 y = 0.0f;
  };

  /* ========================== //
  // Level data structure       //
  // ========================== */
  struct Level
  {
    std::string levelId;
    std::vector<std::string> objectPrefabPaths;
    std::vector<EntityData> entities;
    std::vector<int> loadedPrefabIds; // Track which prefabs were loaded
    std::vector<ECS::EntityID> spawnedEntities; // Track spawned entities
  };
}