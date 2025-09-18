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
     
    if (Engine::Input::IsKeyHeld(GLFW_KEY_S)) vel->x = 10.0f;
    if (Engine::Input::IsKeyHeld(GLFW_KEY_A)) vel->y = 10.0f;
    if (Engine::Input::IsKeyHeld(GLFW_KEY_D)) vel->y = -10.0f;
  }
};
