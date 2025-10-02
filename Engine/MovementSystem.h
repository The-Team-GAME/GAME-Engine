//-----------------------------------------------------------------------------------
// file:    MovementSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a system that handles entity movement by updating
//          positions based on velocity and delta time in the ECS framework.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"
#include "Time.h"
#include <algorithm>
#include <cmath>

class MovementSystem : public ECS::System {
public:
  MovementSystem() {
    require_components<Components::Position, Components::RigidBody>();
  }

  /*=================================================================================
  Function Brief:
  Updates a single entity's position based on its velocity and the fixed delta time.
  This function is called automatically by the ECS framework for each entity that
  has the required components.

  Parameters:
  entity - Reference to the entity being updated.

  Where to call it:
  This is called automatically by the ECS system during the update loop. Do not
  call this directly.
  ==================================================================================*/
  void update_entity(ECS::Entity& entity) override {
    auto* pos = entity.get_component<Components::Position>();
    auto* rb = entity.get_component<Components::RigidBody>();
    auto* col = entity.get_component<Components::Collider>();

    if (!pos || !rb) return;

		if (col->isStatic) return; // Static bodies don't move

    float dt = Engine::Time::get_fixed_dt();

    // Update grounded state based on collision penetration
    updateGroundedState(rb, col);

    // Apply physics forces
    integrateAcceleration(rb, dt);
    applyDrag(rb, dt);
    clampVelocity(rb);
    stopTinyVelocities(rb);

    // Update position
    pos->X() += rb->velocity.X() * dt;
    pos->Y() += rb->velocity.Y() * dt;

    // Reset acceleration for next frame
    rb->acceleration = { 0.0f, 0.0f };
  }

private:
  // Physics constants
  static constexpr f32 MAX_HORIZONTAL_SPEED = 400.0f;
  static constexpr f32 MAX_VERTICAL_SPEED = 1200.0f;
  static constexpr f32 AIR_DRAG = 0.98f;
  static constexpr f32 VELOCITY_THRESHOLD = 0.5f;
  static constexpr f32 GROUND_CHECK_THRESHOLD = 0.02f;

  void updateGroundedState(Components::RigidBody* rb, Components::Collider* col) {
    // Negative Y penetration = ground pushing entity upward (ground below)

    if (rb->velocity.Y() < -100.0f) {
      rb->isGrounded = false;
      return;
    }
    rb->isGrounded = (col && col->penetration.Y() < -GROUND_CHECK_THRESHOLD);
  }

  void integrateAcceleration(Components::RigidBody* rb, f32 dt) {
    rb->velocity.X() += rb->acceleration.X() * dt;
    rb->velocity.Y() += rb->acceleration.Y() * dt;
  }

  void applyDrag(Components::RigidBody* rb, f32 dt) {
    // Horizontal friction (only when grounded and no input)
    if (rb->isGrounded && rb->acceleration.X() == 0.0f) {
      f32 frictionForce = rb->friction * dt;

      if (rb->velocity.X() > 0.0f) {
        rb->velocity.X() = std::max(0.0f, rb->velocity.X() - frictionForce);
      }
      else if (rb->velocity.X() < 0.0f) {
        rb->velocity.X() = std::min(0.0f, rb->velocity.X() + frictionForce);
      }
    }

    // Vertical air drag (always applied)
    rb->velocity.Y() *= AIR_DRAG;
  }

  void clampVelocity(Components::RigidBody* rb) {
    rb->velocity.X() = std::clamp(rb->velocity.X(), -MAX_HORIZONTAL_SPEED, MAX_HORIZONTAL_SPEED);
    rb->velocity.Y() = std::clamp(rb->velocity.Y(), -MAX_VERTICAL_SPEED, MAX_VERTICAL_SPEED);
  }

  void stopTinyVelocities(Components::RigidBody* rb) {
    if (std::abs(rb->velocity.X()) < VELOCITY_THRESHOLD) rb->velocity.X() = 0.0f;
    if (std::abs(rb->velocity.Y()) < VELOCITY_THRESHOLD) rb->velocity.Y() = 0.0f;
  }
};