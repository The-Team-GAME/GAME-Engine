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

      // Zero-initialized before any dynamic initialization runs,
      // so safe to read during static init of other translation units.
      WorldContainers* g_containers = nullptr;

      WorldContainers& init_containers() {
        // Leaked singleton — intentionally never destroyed so that
        // IComponentArray vtables from the DLL remain valid during
        // static cleanup after the DLL has been unloaded.
        static WorldContainers* containers = new WorldContainers();
        g_containers = containers;
        return *containers;
      }

    } // namespace Internal
  } // namespace ECS
} // namespace Engine