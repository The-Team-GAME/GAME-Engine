//-----------------------------------------------------------------------------------
// file:    RotationSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
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
      Transform2DSystem() {
        require_components<Components::Transform2D, Components::RigidBody>();
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* transform = entity.get_component<Components::Transform2D>();
        auto* rigid = entity.get_component<Components::RigidBody>();

        transform->rotation += rigid->angular_velocity * Engine::Time::get_fixed_dt();

        transform->position.x += rigid->linear_velocity.x * Engine::Time::get_fixed_dt();
        transform->position.y += rigid->linear_velocity.y * Engine::Time::get_fixed_dt();
      }
    };
  }
}
