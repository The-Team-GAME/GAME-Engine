#pragma once

#include "../ECS/System.h"
#include "../Components/BasicComponents.h"

class MovementSystem : public ECS::System {
public:
  MovementSystem() {
    requireComponents<Components::Position, Components::Velocity>();
  }

  void updateEntity(ECS::EntityID entityId, float deltaTime) override {
    auto* pos = world->getComponent<Components::Position>(entityId);
    auto* vel = world->getComponent<Components::Velocity>(entityId);

    pos->x += vel->x * deltaTime;
    pos->y += vel->y * deltaTime;
  }
};