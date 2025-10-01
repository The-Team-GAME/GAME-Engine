#pragma once
#include "HealthSystem.h"

class DamageSystem : public ECS::System {
public:
  DamageSystem() {
    listen_to_events<DamageEvent>();
    require_components<Components::Health>();
  }

  void onEvent(const ECS::Event& event, ECS::EntityID entity) override {
    if (const auto* damageEvent = event.get<DamageEvent>()) {
      // The event is for a specific target, only process if it matches the entity this system is currently processing
      if (damageEvent->target == entity) {
        auto* health = world->get_component<Components::Health>(entity);
        health->damage(damageEvent->amount);
      }
    }
  }
};
