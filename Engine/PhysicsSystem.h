#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"
#include "Time.h"
#include "Vector2D.h"
#include <iostream>

class PhysicsSystem : public ECS::System {
public:
  PhysicsSystem() {
    // Require entities to have Position, RigidBody, and Collider
    requireComponents<Components::Position, Components::RigidBody, Components::Collider>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* pos = entity.getComponent<Components::Position>();
    auto* rb = entity.getComponent<Components::RigidBody>();
    auto* col = entity.getComponent<Components::Collider>();

    if (!pos || !rb || !col || rb->isStatic) return;

    float dt = Engine::Time::get_fixed_dt();

    // --- 4. Collision resolution (can use mass here for realistic collisions) ---
    if (col->penetration.x != 0.0f || col->penetration.y != 0.0f) {
      // Push entity out of overlap
      pos->x += col->penetration.x;
      pos->y += col->penetration.y;

      // For more realistic collision response, you could factor in mass:
      // Lighter objects bounce more, heavier objects less
      if (col->penetration.x != 0.0f) {
        rb->velocity.x = -rb->velocity.x * rb->restitution / rb->mass;
      }
      if (col->penetration.y != 0.0f) {
        rb->velocity.y = -rb->velocity.y * rb->restitution / rb->mass;
      }

      col->penetration = { 0.0f, 0.0f };
    }

    // --- 5. Reset acceleration ---
    rb->acceleration = { 0.0f, 0.0f };
  }
};