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

      // i leave this here first
      /*void update_entity(Engine::ECS::Entity& entity) override {
        auto* transform = entity.get_component<Components::Transform2D>();
        auto* rigid = entity.get_component<Components::RigidBody>();

        transform->rotation += rigid->angular_velocity * Engine::Time::get_fixed_dt();

        transform->position.x += rigid->linear_velocity.x * Engine::Time::get_fixed_dt();
        transform->position.y += rigid->linear_velocity.y * Engine::Time::get_fixed_dt();
      }*/

      // Update the velocity here first
      void update_kinematics(Engine::ECS::Entity& entity)
      {
        auto* rigid = entity.get_component<Components::RigidBody>();
        // F = m x a => Force equals mass x acceleration
        rigid->acceleration = rigid->force / rigid->mass;
        // V = a x dt => Velocity equals acceleration x delta time
        rigid->velocity = rigid->acceleration * Time::get_fixed_dt();

        // Checking terminal velocity here
        if (rigid->velocity.get_magnitude_sq() > pow(rigid->terminal_velocity, 2))
        {
          rigid->velocity = Math::normalise_vector(rigid->velocity) * rigid->terminal_velocity;
        }
      }

      // Maybe we check for collision here and then do translate as the last step?
    };
  }
}
