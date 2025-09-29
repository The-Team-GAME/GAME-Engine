#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"

// For now this just clears into a solid color, but could be extended to do more complex background rendering
class GravitySystem : public ECS::System {
public:
  GravitySystem() {
    requireComponents<Components::RigidBody, Components::Gravity>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* rb = entity.getComponent<Components::RigidBody>();
    auto* g = entity.getComponent<Components::Gravity>();

    if (!rb || !g || rb->isStatic) return;

    float dt = Engine::Time::get_fixed_dt();

    // Apply downward acceleration
    rb->acceleration.y += g->strength * 100;
  }
};