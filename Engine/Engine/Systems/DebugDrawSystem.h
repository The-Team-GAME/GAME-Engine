//-----------------------------------------------------------------------------------
// file:    PhysicsSystem.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file defines a system that handles debug illustrations.
//          Currently used for debugging collision
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/ECS.h"
#include "CollisionSystem.h"
namespace Engine {
  namespace Systems {
    class DebugDrawSystem : public Engine::ECS::System 
    {
      public:
        void draw() override;
    };
  }
}
