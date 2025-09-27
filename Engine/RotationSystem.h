#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"
#include "Core.h"

class RotationSystem : public ECS::System {
public:
  RotationSystem() {
    requireComponents<Components::Rotation, Components::RotationSpeed>();
  }

  void updateEntity(ECS::EntityID entityId) override {
    auto* rot = world->getComponent<Components::Rotation>(entityId);
    auto* speed = world->getComponent<Components::RotationSpeed>(entityId);
     
    rot->angle += speed->degreesPerSecond * Engine::Time::get_fixed_dt();
  }
};

