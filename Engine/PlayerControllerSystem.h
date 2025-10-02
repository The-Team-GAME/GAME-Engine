#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"
#include "Time.h"
#include "InputManager.h"

class PlayerControllerSystem : public ECS::System {
public:
  PlayerControllerSystem() {
    require_components<Components::PlayerController, Components::RigidBody, Components::Collider>();
  }

  void update_entity(ECS::Entity& entity) override {
    auto* rb = entity.get_component<Components::RigidBody>();
    //auto* controller = entity.get_component<Components::PlayerController>();
    //auto* col = entity.get_component<Components::Collider>();
    if (!rb) return;

    // Physics constants
    const f32 moveAccel = 2000.0f;      // Horizontal acceleration
    const f32 jumpVelocity = 3000.0f;    // Initial jump velocity
    //const float gravity = 2000.0f;        // Gravity acceleration

    // Apply gravity constantly
    //rb->acceleration.y += gravity;

    // Horizontal movement
    handleHorizontalMovement(rb, moveAccel);

    // Jump
    handleJump(rb, jumpVelocity);
  }

private:
  void handleHorizontalMovement(Components::RigidBody* rb, f32 accel) {
    if (Engine::Input::IsKeyHeld(GLFW_KEY_A)) {
      rb->acceleration.X() -= accel;
    }
    if (Engine::Input::IsKeyHeld(GLFW_KEY_D)) {
      rb->acceleration.X() += accel;
    }
  }

  void handleJump(Components::RigidBody* rb, f32 jumpVel) {
    // Only jump if grounded and space is pressed
		if (rb->isGrounded) std::cout << "Grounded\n";
		else std::cout << "Airborne\n";
    if (Engine::Input::IsKeyPressed(GLFW_KEY_SPACE) && rb->isGrounded) {
			std::cout << "Jump!\n";
      rb->velocity.Y() = -jumpVel;  // Negative if +Y is down
      //rb->isGrounded = false;
    }
  }
};