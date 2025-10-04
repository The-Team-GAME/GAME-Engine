//-----------------------------------------------------------------------------------
// file:    PhysicsSystem.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   This file defines the PhysicsSystem class, which resolves collisions
//          and applies physics-based movement to entities in the ECS framework.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"
#include "Time.h"
#include "Vector2D.h"
#include "TypeDef.h"
#include <iostream>

class PhysicsSystem : public ECS::System {
public:
  PhysicsSystem() {
    require_components<Components::Position, Components::RigidBody, Components::Collider>();
  }

  void update_entity(ECS::Entity& entity) override {
    auto* pos = entity.get_component<Components::Position>();
    auto* rb = entity.get_component<Components::RigidBody>();
    auto* col = entity.get_component<Components::Collider>();

    if (!pos || !rb || !col) return;
    if (col->isStatic) return; // Static bodies don't move

    f32 dt = Engine::Time::get_fixed_dt();

    // --- Handle Dynamic Collisions (Swept AABB) ---
    if (col->dynamicCollisionTime >= 0.0f && col->dynamicCollisionTime <= dt) {
      Math::Vec2D normal = col->dynamicNormal;
      f32 dot_product = rb->velocity.x * normal.x + rb->velocity.y * normal.y;

      // Only resolve if moving into the collision
      if (dot_product < 0.0f) {
        // CRITICAL: Rewind position to collision point
        // MovementSystem already moved us the full dt, so we need to undo that
        // and move only to the collision time
        f32 overshoot = dt - col->dynamicCollisionTime;
        pos->x -= rb->velocity.x * overshoot;
        pos->y -= rb->velocity.y * overshoot;

        // Add small separation to prevent sinking
        const f32 separation_bias = 0.1f;
        pos->x += normal.x * separation_bias;
        pos->y += normal.y * separation_bias;

        // Reflect velocity along collision normal (with restitution)
        rb->velocity.x -= (1.0f + rb->restitution) * dot_product * normal.x;
        rb->velocity.y -= (1.0f + rb->restitution) * dot_product * normal.y;

        // Apply friction on tangent
        Math::Vec2D tangent = { -normal.y, normal.x };
        f32 tangent_vel = rb->velocity.x * tangent.x + rb->velocity.y * tangent.y;
        rb->velocity.x -= tangent_vel * rb->friction * tangent.x;
        rb->velocity.y -= tangent_vel * rb->friction * tangent.y;
      }

      // Reset collision data
      col->dynamicCollisionTime = -1.0f;
      col->dynamicNormal = { 0.0f, 0.0f };

      return;
    }

    // --- Handle Static Collisions (Penetration Resolution) ---
    if (col->penetration.x != 0.0f || col->penetration.y != 0.0f) {
      // Ignore very small overlaps
      col->penetration.x = (fabs(col->penetration.x) < 3.f) ? 0.0f : col->penetration.x;
      col->penetration.y = (fabs(col->penetration.y) < 3.f) ? 0.0f : col->penetration.y;

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