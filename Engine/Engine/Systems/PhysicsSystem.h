//-----------------------------------------------------------------------------------
// file:    PhysicsSystem.h
// project: Monkey Stole My GPA
//
// author:   (100%)
//          Primary author
//
// email:   @digipen.edu
//
// brief:   This file defines a system that handles entity rotation by updating
//          rotation angles based on rotation speed and delta time in the ECS
//          framework.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once


#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Time.h"

namespace Engine {
  namespace Systems
  {
    class Transform2DSystem : public Engine::ECS::System {
    public:
      Transform2DSystem();; // Default dtor that includes all the components needed for the entities called in the physics engine

      // Update the linear velocities if said entities here
      void update_kinematics(Engine::ECS::Entity& entity);

      // Maybe we check for collision here and then do translate as the last step?
    };
  }
}
