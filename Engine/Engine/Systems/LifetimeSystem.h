//-----------------------------------------------------------------------------------
// file:    LifetimeSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   System that handles entity lifetime management. Entities with a Lifetime
//          component are automatically destroyed after their duration expires.
//          Useful for temporary entities like projectiles, effects, and hitboxes.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Time.h"

namespace Engine
{
  namespace Systems
  {
    class LifetimeSystem : public Engine::ECS::System {
    public:
      LifetimeSystem() {
        require_components<Components::Lifetime>();
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* lifetime = entity.get_component<Components::Lifetime>();

        if (lifetime) {
          lifetime->elapsed += Engine::Time::get_fixed_dt();

          // Destroy entity if lifetime has expired
          if (lifetime->elapsed >= lifetime->duration) {
            Engine::ECS::destroy_entity(entity);
          }
        }
      }
    };

  } // namespace Systems
} // namespace Engine
