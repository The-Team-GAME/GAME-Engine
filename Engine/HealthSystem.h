#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"

class HealthSystem : public ECS::System
{
public:
  HealthSystem()
  {
    requireComponents<Components::Health>();
  }

  void updateEntity(ECS::EntityID entityId) override
  {

    auto* health = world->getComponent<Components::Health>(entityId);

    if (health->isDead())
    {
      world->getEntity(entityId).destroy();
    }
  }
};