#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"
#include "InputManager.h"

class PlayerControllerSystem : public ECS::System {
public:
  PlayerControllerSystem() {
    requireComponents<Components::PlayerController, Components::RigidBody>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* rb = entity.getComponent<Components::RigidBody>( );
    if (!rb || rb->isStatic) return; // skip static objects

    const float accelAmount = 500.0f; // acceleration per second

    // Reset acceleration is optional here; PhysicsSystem also resets it each frame
    // rb->acceleration = {0.0f, 0.0f};

    // Apply player input as acceleration
    if (Engine::Input::IsKeyHeld(GLFW_KEY_W)) rb->acceleration.y -= accelAmount;
    if (Engine::Input::IsKeyHeld(GLFW_KEY_S)) rb->acceleration.y += accelAmount;
    if (Engine::Input::IsKeyHeld(GLFW_KEY_A)) rb->acceleration.x -= accelAmount;
    if (Engine::Input::IsKeyHeld(GLFW_KEY_D)) rb->acceleration.x += accelAmount;
  }
};
