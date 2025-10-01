#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"
#include "Time.h"
#include "Vector2D.h"
#include <iostream>

class PhysicsSystem : public ECS::System {
public:
  PhysicsSystem() {
    requireComponents<Components::Position, Components::RigidBody, Components::Collider>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* pos = entity.getComponent<Components::Position>();
    auto* rb = entity.getComponent<Components::RigidBody>();
    auto* col = entity.getComponent<Components::Collider>();

    if (!pos || !rb || !col) return;
    if (col->isStatic) return; // Static bodies don't move

    float dt = Engine::Time::get_fixed_dt();

    // --- Handle Dynamic Collisions (Swept AABB) ---
    if (col->dynamicCollisionTime >= 0.0f && col->dynamicCollisionTime <= dt) {
      Math::Vec2D normal = col->dynamicNormal;
      float dotProduct = rb->velocity.x * normal.x + rb->velocity.y * normal.y;

      // Only resolve if moving into the collision
      if (dotProduct < 0.0f) {
        // CRITICAL: Rewind position to collision point
        // MovementSystem already moved us the full dt, so we need to undo that
        // and move only to the collision time
        float overshoot = dt - col->dynamicCollisionTime;
        pos->x -= rb->velocity.x * overshoot;
        pos->y -= rb->velocity.y * overshoot;

        // Add small separation to prevent sinking
        const float SEPARATION_BIAS = 0.1f;
        pos->x += normal.x * SEPARATION_BIAS;
        pos->y += normal.y * SEPARATION_BIAS;

        // Reflect velocity along collision normal (with restitution)
        rb->velocity.x -= (1.0f + rb->restitution) * dotProduct * normal.x;
        rb->velocity.y -= (1.0f + rb->restitution) * dotProduct * normal.y;

        // Apply friction on tangent
        Math::Vec2D tangent = { -normal.y, normal.x };
        float tangentVel = rb->velocity.x * tangent.x + rb->velocity.y * tangent.y;
        rb->velocity.x -= tangentVel * rb->friction * tangent.x;
        rb->velocity.y -= tangentVel * rb->friction * tangent.y;
      }

      // Reset collision data
      col->dynamicCollisionTime = -1.0f;
      col->dynamicNormal = { 0.0f, 0.0f };

      return;
    }

    // --- Handle Static Collisions (Penetration Resolution) ---
    if (col->penetration.x != 0.0f || col->penetration.y != 0.0f) {
      // Ignore very small overlaps
      col->penetration.x = (fabs(col->penetration.x) < 0.1f) ? 0.0f : col->penetration.x;
      col->penetration.y = (fabs(col->penetration.y) < 0.1f) ? 0.0f : col->penetration.y;

      // Push entity out of overlap
      pos->x += col->penetration.x;
      pos->y += col->penetration.y;

      // Bounce velocity based on penetration direction
      if (col->penetration.x != 0.0f) {
        rb->velocity.x = -rb->velocity.x * rb->restitution;
      }
      if (col->penetration.y != 0.0f) {
        rb->velocity.y = -rb->velocity.y * rb->restitution;
      }

      col->penetration = { 0.0f, 0.0f };
      return;
    }
  }
};