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

  void updateEntity(ECS::EntityID entityId) override {
    auto* pos = world->getComponent<Components::Position>(entityId);
    auto* rb = world->getComponent<Components::RigidBody>(entityId);
    auto* col = world->getComponent<Components::Collider>(entityId);

    if (!pos || !rb || !col || rb->isStatic) return;

    float dt = Engine::Time::get_fixed_dt();

    // --- 1. Integrate acceleration into velocity ---
    //if (rb->useGravity) {
    //  rb->acceleration.y += -9.81f * dt; // simple gravity
    //}
    rb->velocity.x += rb->acceleration.x * dt;
    rb->velocity.y += rb->acceleration.y * dt;

    // --- 2. Integrate velocity into position ---
    rb->velocity.x += rb->acceleration.x * dt;
    rb->velocity.y += rb->acceleration.y * dt;

    // --- 3. Integrate velocity into position ---
    pos->x += rb->velocity.x * dt;
    pos->y += rb->velocity.y * dt;

    // --- 4. Collision resolution (can use mass here for realistic collisions) ---
    if (col->penetration.x != 0.0f || col->penetration.y != 0.0f) {
      // Push entity out of overlap
      pos->x += col->penetration.x;
      pos->y += col->penetration.y;

      // For more realistic collision response, you could factor in mass:
      // Lighter objects bounce more, heavier objects less
      float restitution = 0.0f; // 0 = no bounce, 1 = perfect bounce
      if (col->penetration.x != 0.0f) {
        rb->velocity.x = -rb->velocity.x * restitution / rb->mass;
      }
      if (col->penetration.y != 0.0f) {
        rb->velocity.y = -rb->velocity.y * restitution / rb->mass;
      }

      col->penetration = { 0.0f, 0.0f };
    }

    // --- 5. Reset acceleration ---
    rb->acceleration = { 0.0f, 0.0f };
  }
};