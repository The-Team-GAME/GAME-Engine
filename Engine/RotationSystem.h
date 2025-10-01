#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"
#include "Core.h"

class RotationSystem : public ECS::System {
public:
  RotationSystem() {
    require_components<Components::Rotation, Components::RotationSpeed>();
  }

  void update_entity(ECS::Entity& entity) override {
    auto* rot = entity.get_component<Components::Rotation>();
    auto* speed = entity.get_component<Components::RotationSpeed>();

    rot->angle += speed->degrees_per_second * Engine::Time::get_fixed_dt();
  }
};

