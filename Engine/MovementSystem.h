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

    pos->x += vel->x * Engine::Time::get_fixed_dt();
    pos->y += vel->y * Engine::Time::get_fixed_dt();
  }
};