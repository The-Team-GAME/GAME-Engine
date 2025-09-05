#pragma once
#include "../ECS/System.h"
#include "../Components/BasicComponents.h"

class HealthSystem : public ECS::System {
public:
  HealthSystem() {
    requireComponents<Components::Health>();
  }

  void updateEntity(ECS::EntityID entityId, float deltaTime) override {
    auto* health = world->getComponent<Components::Health>(entityId);

    if (health->isDead()) {
      world->getEntity(entityId).destroy();
    }
  }
};