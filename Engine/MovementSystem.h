#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"

class MovementSystem : public ECS::System {
public:
  MovementSystem() {
    requireComponents<Components::Position, Components::Velocity>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* pos = entity.getComponent<Components::Position>();
    auto* vel = entity.getComponent<Components::Velocity>();

    pos->X() += vel->X() * Engine::Time::get_fixed_dt();
    pos->Y() += vel->Y() * Engine::Time::get_fixed_dt();
  }
};