#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"
#include "Time.h"
#include <algorithm>
#include <cmath>

class MovementSystem : public ECS::System {
public:
  MovementSystem() {
    requireComponents<Components::Position, Components::RigidBody>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* pos = entity.getComponent<Components::Position>();
    auto* rb = entity.getComponent<Components::RigidBody>();
    auto* col = entity.getComponent<Components::Collider>();

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
    pos->x += rb->velocity.x * dt;
    pos->y += rb->velocity.y * dt;

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

    if (rb->velocity.y < -100.0f) {
      rb->isGrounded = false;
      return;
    }
    rb->isGrounded = (col && col->penetration.y < -GROUND_CHECK_THRESHOLD);
  }

  void integrateAcceleration(Components::RigidBody* rb, f32 dt) {
    rb->velocity.x += rb->acceleration.x * dt;
    rb->velocity.y += rb->acceleration.y * dt;
  }

  void applyDrag(Components::RigidBody* rb, f32 dt) {
    // Horizontal friction (only when grounded and no input)
    if (rb->isGrounded && rb->acceleration.x == 0.0f) {
      f32 frictionForce = rb->friction * dt;

      if (rb->velocity.x > 0.0f) {
        rb->velocity.x = std::max(0.0f, rb->velocity.x - frictionForce);
      }
      else if (rb->velocity.x < 0.0f) {
        rb->velocity.x = std::min(0.0f, rb->velocity.x + frictionForce);
      }
    }

    // Vertical air drag (always applied)
    rb->velocity.y *= AIR_DRAG;
  }

  void clampVelocity(Components::RigidBody* rb) {
    rb->velocity.x = std::clamp(rb->velocity.x, -MAX_HORIZONTAL_SPEED, MAX_HORIZONTAL_SPEED);
    rb->velocity.y = std::clamp(rb->velocity.y, -MAX_VERTICAL_SPEED, MAX_VERTICAL_SPEED);
  }

  void stopTinyVelocities(Components::RigidBody* rb) {
    if (std::abs(rb->velocity.x) < VELOCITY_THRESHOLD) rb->velocity.x = 0.0f;
    if (std::abs(rb->velocity.y) < VELOCITY_THRESHOLD) rb->velocity.y = 0.0f;
  }
};