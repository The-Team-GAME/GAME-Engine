#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"
#include "Time.h"
#include <algorithm> // for std::max

class MovementSystem : public ECS::System {
public:
  MovementSystem() {
    requireComponents<Components::Position, Components::RigidBody>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* pos = entity.getComponent<Components::Position>();
    auto* rb = entity.getComponent<Components::RigidBody>();
    auto* col = entity.getComponent<Components::Collider>();

    if (!pos || !rb || rb->isStatic) return;

    float dt = Engine::Time::get_fixed_dt();

    // Physics constants
    const float maxHorizontalSpeed = 400.0f;
    const float maxVerticalSpeed = 1200.0f;
    const float airDrag = 0.98f;           // Multiplier per frame (closer to 1.0 = less drag)
    const float stopThreshold = 0.5f;      // Velocity below this becomes 0

    // Check if grounded based on collision (downward penetration)
    bool isGrounded = false;
    if (col && col->penetration.y < -0.01f) { // Negative Y penetration = ground below
      isGrounded = true;
      if (rb) {
        rb->isGrounded = true;
      }
    }

    // Track horizontal input before integration
    bool hasHorizontalInput = (rb->acceleration.x != 0.0f);

    // 1. Integrate acceleration into velocity
    integrateAcceleration(rb, dt);

    // 2. Apply friction only when grounded
    if (isGrounded && !hasHorizontalInput) {
      applyFriction(rb, dt);
    }

    // 3. Apply air drag (always)
    applyAirDrag(rb, airDrag);

    // 4. Clamp velocity to max speeds
    clampVelocity(rb, maxHorizontalSpeed, maxVerticalSpeed);

    // 5. Stop tiny velocities (prevent jitter)
    stopTinyVelocities(rb, stopThreshold);

    // 6. Update position based on velocity
    updatePosition(pos, rb, dt);

    // 7. Reset acceleration for next frame
    rb->acceleration = { 0.0f, 0.0f };
  }

private:
  void integrateAcceleration(Components::RigidBody* rb, float dt) {
    rb->velocity.x += rb->acceleration.x * dt;
    rb->velocity.y += rb->acceleration.y * dt;
  }

  void applyFriction(Components::RigidBody* rb, float dt) {
    // Only apply friction when no horizontal input (acceleration is 0)
    if (rb->acceleration.x == 0.0f) {
      float frictionForce = rb->friction;

      if (rb->velocity.x > 0.0f) {
        rb->velocity.x = std::max(0.0f, rb->velocity.x - frictionForce);
      }
      else if (rb->velocity.x < 0.0f) {
        rb->velocity.x = std::min(0.0f, rb->velocity.x + frictionForce);
      }
    }
  }

  void applyAirDrag(Components::RigidBody* rb, float dragMultiplier) {
    // Gentle vertical drag (use multiplier instead of subtraction)
    rb->velocity.y *= dragMultiplier;
  }

  void clampVelocity(Components::RigidBody* rb, float maxX, float maxY) {
    rb->velocity.x = std::clamp(rb->velocity.x, -maxX, maxX);
    rb->velocity.y = std::clamp(rb->velocity.y, -maxY, maxY);
  }

  void stopTinyVelocities(Components::RigidBody* rb, float threshold) {
    if (std::abs(rb->velocity.x) < threshold) rb->velocity.x = 0.0f;
    if (std::abs(rb->velocity.y) < threshold) rb->velocity.y = 0.0f;
  }

  void updatePosition(Components::Position* pos, Components::RigidBody* rb, float dt) {
    pos->x += rb->velocity.x * dt;
    pos->y += rb->velocity.y * dt;
  }
};