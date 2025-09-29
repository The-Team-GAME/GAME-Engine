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
    auto* rb = entity.getComponent<Components::RigidBody>();
    auto* controller = entity.getComponent<Components::PlayerController>();

    if (!rb || rb->isStatic) return;

    // Physics constants
    const float moveAccel = 2000.0f;      // Horizontal acceleration
    const float jumpVelocity = 2000.0f;    // Initial jump velocity
    //const float gravity = 2000.0f;        // Gravity acceleration

    // Apply gravity constantly
    //rb->acceleration.y += gravity;

    // Horizontal movement
    handleHorizontalMovement(rb, moveAccel);

    // Jump
    handleJump(rb, jumpVelocity);
  }

private:
  void handleHorizontalMovement(Components::RigidBody* rb, float accel) {
    if (Engine::Input::IsKeyHeld(GLFW_KEY_A)) {
      rb->acceleration.x -= accel;
    }
    if (Engine::Input::IsKeyHeld(GLFW_KEY_D)) {
      rb->acceleration.x += accel;
    }
  }

  void handleJump(Components::RigidBody* rb, float jumpVel) {
    // Only jump if grounded and space is pressed
    if (Engine::Input::IsKeyPressed(GLFW_KEY_SPACE) && rb->isGrounded) {
      rb->velocity.y = -jumpVel;  // Negative if +Y is down
      rb->isGrounded = false;
    }
  }
};