

#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"
#include "Core.h"

class SimplePlayerMovementSystem : public ECS::System {
public:
  SimplePlayerMovementSystem() {
    require_components<Components::Position, Components::SimplePlayerControl>();
  }

  void update_entity(ECS::Entity& entity) override {
    auto* pos = entity.get_component<Components::Position>();
    auto* control = entity.get_component<Components::SimplePlayerControl>();

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

