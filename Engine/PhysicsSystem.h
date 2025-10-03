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
      f32 dot_product = rb->velocity.X() * normal.X() + rb->velocity.Y() * normal.Y();

      // Only resolve if moving into the collision
      if (dot_product < 0.0f) {
        // CRITICAL: Rewind position to collision point
        // MovementSystem already moved us the full dt, so we need to undo that
        // and move only to the collision time
        f32 overshoot = dt - col->dynamicCollisionTime;
        pos->X() -= rb->velocity.X() * overshoot;
        pos->Y() -= rb->velocity.Y() * overshoot;

        // Add small separation to prevent sinking
        const f32 separation_bias = 0.1f;
        pos->X() += normal.X() * separation_bias;
        pos->Y() += normal.Y() * separation_bias;

        // Reflect velocity along collision normal (with restitution)
        rb->velocity.X() -= (1.0f + rb->restitution) * dot_product * normal.X();
        rb->velocity.Y() -= (1.0f + rb->restitution) * dot_product * normal.Y();

        // Apply friction on tangent
        Math::Vec2D tangent = { -normal.Y(), normal.X() };
        f32 tangent_vel = rb->velocity.X() * tangent.X() + rb->velocity.Y() * tangent.Y();
        rb->velocity.X() -= tangent_vel * rb->friction * tangent.X();
        rb->velocity.Y() -= tangent_vel * rb->friction * tangent.Y();
      }

      // Reset collision data
      col->dynamicCollisionTime = -1.0f;
      col->dynamicNormal = { 0.0f, 0.0f };

      return;
    }

    // --- Handle Static Collisions (Penetration Resolution) ---
    if (col->penetration.X() != 0.0f || col->penetration.Y() != 0.0f) {
      // Ignore very small overlaps
      col->penetration.X() = (fabs(col->penetration.X()) < 3.f) ? 0.0f : col->penetration.X();
      col->penetration.Y() = (fabs(col->penetration.Y()) < 3.f) ? 0.0f : col->penetration.Y();

      // Push entity out of overlap
      pos->X() += col->penetration.X();
      pos->Y() += col->penetration.Y();

      // Bounce velocity based on penetration direction
      if (col->penetration.X() != 0.0f) {
        rb->velocity.X() = -rb->velocity.X() * rb->restitution;
      }
      if (col->penetration.Y() != 0.0f) {
        rb->velocity.Y() = -rb->velocity.Y() * rb->restitution;
      }

      col->penetration = { 0.0f, 0.0f };
      return;
    }
  }
};