#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"

class MovementSystem : public ECS::System {
public:
  MovementSystem() {
    requireComponents<Components::Position, Components::Velocity>();
  }

  void updateEntity(ECS::EntityID entityId) override {
    auto* pos = world->getComponent<Components::Position>(entityId);
    auto* vel = world->getComponent<Components::Velocity>(entityId);

    pos->x += vel->x * Engine::CORE->getDt();
    pos->y += vel->y * Engine::CORE->getDt();
  }
};