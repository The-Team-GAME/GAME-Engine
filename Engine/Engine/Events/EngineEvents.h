//-----------------------------------------------------------------------------------
// file:    EngineEvents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines engine-level event structures. Includes entity
//          selection events for synchronizing selection state across systems
//          and UI panels.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "../TypeDef.h"
#include "../ECS/ECS.h"

namespace Engine {
  namespace Events {

    /* ======================= //
    // Entity selection events //
    // ======================= */
    struct SelectEntityEvent {
      Engine::ECS::EntityID entity_id;
    };

  } // namespace Events
} // namespace Engine
