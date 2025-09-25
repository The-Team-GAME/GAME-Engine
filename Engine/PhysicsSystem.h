#pragma once


#include "ECS/System.h"
#include "BasicComponents.h"
#include "CollisionSystem.h"
#include "Time.h"
#include "Core.h"
#include "Vector2D.h"


//Ryan: this shit dont work rn just the collision system
class PhysicsSystem : public ECS::System {
public:
  PhysicsSystem() {
    // Require entities to have Position and RigidBody
    requireComponents<Components::Position, Components::RigidBody>();
  }

  void updateEntity(ECS::EntityID entityId) override {
    auto* pos = world->getComponent<Components::Position>(entityId);
    auto* rb = world->getComponent<Components::RigidBody>(entityId);

    if (!pos || !rb) return;
    if (rb->isStatic) return; // Skip static objects

    // 1?? Apply velocity & acceleration (Euler integration)
    rb->velocity += rb->acceleration;   // v = v + a
    pos->x += rb->velocity.x;           // p = p + v
    pos->y += rb->velocity.y;

    // 2?? Reset acceleration for next frame
    rb->acceleration = { 0.0f, 0.0f };

    // Optional: gravity
    // rb->acceleration.y += -9.81f * Time::deltaTime; 
  }

  // Optional: integrate collisions from CollisionSystem
 
};