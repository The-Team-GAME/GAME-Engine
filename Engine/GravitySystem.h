#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"

// For now this just clears into a solid color, but could be extended to do more complex background rendering
class GravitySystem : public ECS::System {
public:
  GravitySystem() {
    requireComponents<Components::RigidBody, Components::Gravity, Components::Collider>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* rb = entity.getComponent<Components::RigidBody>();
    auto* g = entity.getComponent<Components::Gravity>();
    auto* col = entity.getComponent<Components::Collider>();

    if (!rb || !g) return;
		if (col->isStatic) return; // Static bodies don't move

    float dt = Engine::Time::get_fixed_dt();

    // Apply downward acceleration
    if (rb->isGrounded) return; // No gravity if grounded
    rb->acceleration.y += g->strength * 12000 * dt;
  }
};