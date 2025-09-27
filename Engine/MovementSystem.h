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

    if (!pos || !rb) return;
    if (rb->isStatic) return; // skip static bodies

    float dt = Engine::Time::get_fixed_dt();

    // --- 1. Integrate acceleration into velocity ---
    rb->velocity.x += rb->acceleration.x * dt;
    rb->velocity.y += rb->acceleration.y * dt;

    // --- 2. Apply friction ---
    // Friction reduces velocity along each axis
    rb->velocity.x -= rb->velocity.x * rb->friction * dt;
    rb->velocity.y -= rb->velocity.y * rb->friction * dt;

    // Optional: stop tiny velocities to avoid jitter
    if (std::abs(rb->velocity.x) < 0.01f) rb->velocity.x = 0.0f;
    if (std::abs(rb->velocity.y) < 0.01f) rb->velocity.y = 0.0f;

    // --- 3. Update position ---
    pos->x += rb->velocity.x * dt;
    pos->y += rb->velocity.y * dt;

    // --- 4. Reset acceleration for next frame ---
    rb->acceleration = { 0.0f, 0.0f };
  }
};
