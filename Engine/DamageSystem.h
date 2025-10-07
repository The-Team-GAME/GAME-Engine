//-----------------------------------------------------------------------------------
// file:    DamageSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the DamageSystem which handles DamageEvent processing
//          for entities with Health components. Applies damage to targeted entities
//          when damage events are dispatched.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "HealthSystem.h"

class DamageSystem : public ECS::System {
public:
  DamageSystem() {
    listen_to_events<DamageEvent>();
    require_components<Components::Health>();
  }

  void on_event(const ECS::Event& event, ECS::EntityID entity) override {
    if (const auto* damageEvent = event.get<DamageEvent>()) {
      // The event is for a specific target, only process if it matches the entity this system is currently processing
      if (damageEvent->target == entity) {
        auto* health = world->get_component<Components::Health>(entity);
        health->damage(damageEvent->amount);
      }
    }
  }
};
