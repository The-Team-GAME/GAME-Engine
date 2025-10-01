#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"

class MovementSystem : public ECS::System {
public:
  MovementSystem() {
    require_components<Components::Position, Components::Velocity>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* pos = entity.get_component<Components::Position>();
    auto* vel = entity.get_component<Components::Velocity>();

    pos->X() += vel->X() * Engine::Time::get_fixed_dt();
    pos->Y() += vel->Y() * Engine::Time::get_fixed_dt();
  }
};