#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"
#include "InputManager.h"

class PlayerControllerSystem : public ECS::System {
public:
  PlayerControllerSystem() {
    requireComponents<Components::PlayerController, Components::Velocity>();
  }

  void updateEntity(ECS::EntityID entityId) override {
    auto* vel = world->getComponent<Components::Velocity>(entityId);
    auto* ctrl = world->getComponent<Components::PlayerController>(entityId);

    // Reset velocity each frame
    vel->x = 0.0f;
    vel->y = 0.0f;

    const float speed = 100.0f;

    // Movement mapping
    if (Engine::Input::IsKeyHeld(GLFW_KEY_W)) vel->y -= speed;  // Up
    if (Engine::Input::IsKeyHeld(GLFW_KEY_S)) vel->y += speed;  // Down
    if (Engine::Input::IsKeyHeld(GLFW_KEY_A)) vel->x -= speed;  // Left
    if (Engine::Input::IsKeyHeld(GLFW_KEY_D)) vel->x += speed;  // Right
  }
};
