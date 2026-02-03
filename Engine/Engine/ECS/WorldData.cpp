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
//          Uses a heap-allocated object so we can control destruction timing.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "WorldData.h"
#include "../Logger.h"

namespace Engine {
  namespace ECS {
    namespace Internal {

      // Use a pointer so we can control destruction timing
      // This avoids false positive memory leak reports from CRT
      static WorldContainers* s_containers = nullptr;

      WorldContainers& get_containers() {
        if (!s_containers) {
          // Use operator new directly to avoid going through MemoryManager
          // (which may not be initialized yet during static init)
          s_containers = new WorldContainers();
          Log::memory("WorldContainers created at {}", static_cast<void*>(s_containers));
        }
        return *s_containers;
      }

    } // namespace Internal
  } // namespace ECS
} // namespace Engine