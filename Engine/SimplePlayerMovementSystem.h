

#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"
#include "Core.h"

class SimplePlayerMovementSystem : public ECS::System {
public:
  SimplePlayerMovementSystem() {
    requireComponents<Components::Position, Components::SimplePlayerControl>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* pos = entity.getComponent<Components::Position>();
    auto* control = entity.getComponent<Components::SimplePlayerControl>();

    if (Engine::Input::IsKeyHeld(GLFW_KEY_W)) {
      pos->Y() -= control->speed * Engine::Time::get_fixed_dt();
    }
    if (Engine::Input::IsKeyHeld(GLFW_KEY_S)) {
      pos->Y() += control->speed * Engine::Time::get_fixed_dt();
    }
    if (Engine::Input::IsKeyHeld(GLFW_KEY_A)) {
      pos->X() -= control->speed * Engine::Time::get_fixed_dt();
    }
    if (Engine::Input::IsKeyHeld(GLFW_KEY_D)) {
      pos->X() += control->speed * Engine::Time::get_fixed_dt();
    }
  }
};

