//-----------------------------------------------------------------------------------
// file:    EnemyHealthSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   System that manages enemy health and triggers state transitions when damaged
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"
#include "../Components/EnemyHealthComponent.h"
#include "../Components/HealthComponents.h"

namespace Engine {
  namespace Systems
  {
    class EnemyHealthSystem : public Engine::ECS::System {
    public:
      EnemyHealthSystem() {
        require_components<Components::EnemyHealth>();
      }

      void update_entity(Engine::ECS::Entity entity) override {
        auto* enemy_health = entity.get_component<Components::EnemyHealth>();
        if (!enemy_health) return;

        if (!entity.has_component<Components::EnemyHealthBarRect>()) {
          entity.add_component<Components::EnemyHealthBarRect>();
        }

        // Check if enemy is dead
        if (enemy_health->is_dead()) {
          // Destroy the enemy entity
          // Engine::ECS::destroy_entity(entity);
          // Log::info("Enemy destroyed!");
        }
      }
    };
  }
}