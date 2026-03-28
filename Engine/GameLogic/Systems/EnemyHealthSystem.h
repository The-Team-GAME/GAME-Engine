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
#include "../Components/CatMonkeyBossComponent.h"

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

        // Most enemies get a small overhead HP bar. Cat Monkey uses the
        // 5 large bottom-screen wave bars instead, so suppress the overhead bar.
        if (!entity.has_component<Components::EnemyHealthBarRect>()
          && !entity.has_component<Components::CatMonkeyBoss>()) {
          entity.add_component<Components::EnemyHealthBarRect>();
        }

        // Check if enemy is dead and ensure its state machine enters the Death state
        if (enemy_health->is_dead()) {
          if (auto* sm = entity.get_component<Components::StateMachine>()) {
            if (sm->get_current_state() != "Death") {
              sm->change_state("Death");
            }
          }
        }
      }
    };
  }
}