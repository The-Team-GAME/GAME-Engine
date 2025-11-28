//-----------------------------------------------------------------------------------
// file:    PlayerControllerSystem.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   This file defines the PlayerControllerSystem class, which handles
//          player input and applies movement and jump forces to entities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"
#include "Time.h"
#include "InputManager.h"

class PlayerControllerSystem : public Engine::ECS::System {
public:
  PlayerControllerSystem() {
    require_components<Components::PlayerController, Components::RigidBody, Components::Collider>();
  }

  void update_entity(Engine::ECS::Entity& entity) override {
    auto* rb = entity.get_component<Components::RigidBody>();
    //auto* controller = entity.get_component<Components::PlayerController>();
    //auto* col = entity.get_component<Components::Collider>();
    if (!rb) return;

    // Physics constants
    const f32 moveAccel = 2000.0f;      // Horizontal acceleration
    const f32 jumpVelocity = 3000.0f;    // Initial jump velocity
    //const f32 gravity = 2000.0f;        // Gravity acceleration

    // Apply gravity constantly
    //rb->acceleration.y += gravity;

    // Horizontal movement
    handle_horizontal_movement(rb, moveAccel);

    // Jump
    handle_jump(rb, jumpVelocity);
  }

private:
  void handle_horizontal_movement(Components::RigidBody* rb, f32 accel) {
    if (Engine::Input::is_held(GLFW_KEY_A)) {
      rb->acceleration.x -= accel;
    }
    if (Engine::Input::is_held(GLFW_KEY_D)) {
      rb->acceleration.x += accel;
    }
  }

    void handle_jump(Components::RigidBody* rb, f32 jumpVel) {
    // Only jump if grounded and space is pressed
    if (rb->isGrounded) Log::info("Grounded");
    else Log::info("Airborne");
    if (Engine::Input::is_pressed(GLFW_KEY_SPACE) && rb->isGrounded) {
      Log::info("Jump!");
      rb->velocity.y = -jumpVel;  // Negative if +Y is down
      //rb->isGrounded = false;
    }
  }
};