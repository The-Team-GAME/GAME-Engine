//-----------------------------------------------------------------------------------
// file:    WorldData.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the centralized container accessor for the ECS World.
//          Uses a static local variable to ensure single instance initialization.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "WorldData.h"

namespace Engine {
  namespace ECS {
    namespace Internal {

      WorldContainers* g_containers = nullptr;

      WorldContainers& init_containers() {
        g_containers = new WorldContainers();
        return *g_containers;
      }

    } // namespace Internal
  } // namespace ECS
} // namespace Engine