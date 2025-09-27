#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"
#include "Core.h"

class RotationSystem : public ECS::System {
public:
  RotationSystem() {
    requireComponents<Components::Rotation, Components::RotationSpeed>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* rot = entity.getComponent<Components::Rotation>();
    auto* speed = entity.getComponent<Components::RotationSpeed>();

    rot->angle += speed->degreesPerSecond * Engine::Time::get_fixed_dt();
  }
};

