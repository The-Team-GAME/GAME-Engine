//-----------------------------------------------------------------------------------
// file:    PhysicsSystem.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file defines a system that handles the post physics updates for 
//          all entities tht contain rigidbody and transform2d components.
//          Entities will have their rigidbody updated which in turn updates the
//          transform2d component as well.
//          This system is designed to run after the Collision2DSystem before the 
//          object gets rendered
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "PhysicsSystem.h"
namespace Engine {
  namespace Systems
  {
    class PostPhysics2DSystem : public Engine::ECS::System
    {
    public:
      PostPhysics2DSystem(); //Default Ctor

      void update_entity(Engine::ECS::Entity& entity) override;
    };
  }
}

