//-----------------------------------------------------------------------------------
// file:    UtilitiesSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   System that handles entity lifetime management. Entities with a Lifetime
//          component are automatically destroyed at a specific time. Uses
//          Engine::Time::get_time() to check when to destroy. More efficient than
//          tracking elapsed time. Useful for temporary entities like projectiles,
//          effects, and hitboxes.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"

namespace Engine
{
  namespace Systems
  {
    class LifetimeSystem : public Engine::ECS::System {
    public:
      LifetimeSystem() {
        require_components<Components::Lifetime>();
      }

      void update_entity(Engine::ECS::Entity entity) override {
        auto* lifetime = entity.get_component<Components::Lifetime>();

        if (lifetime) {
          // Check if it's time to destroy the entity
          if (Engine::Time::get_time() >= lifetime->destroy_at) {
            Engine::ECS::destroy_entity(entity);
          }
        }
      }
    };

    class FrameLifetimeSystem : public Engine::ECS::System {
    public:
      FrameLifetimeSystem() {
        require_components<Components::FrameLifetime>();
      }

      void update_entity(Engine::ECS::Entity entity) override {
        auto* frame_lifetime = entity.get_component<Components::FrameLifetime>();

        if (frame_lifetime) {
          // Decrement frames remaining
          frame_lifetime->frames_remaining--;

          // Destroy entity when frames remaining reaches 0 or below
          // This means frames_remaining=1 will last exactly 1 update cycle:
          // - Creation: frames_remaining = 1
          // - Frame 0 Update: Decrements to 0, then destroys
          if (frame_lifetime->frames_remaining <= 0) {
            Engine::ECS::destroy_entity(entity);
          }
        }
      }
    };

  } // namespace Systems
} // namespace Engine

