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

  void updateEntity(ECS::Entity& entity) override
  {
    auto health = entity.getComponent<Components::Health>();

    if (health->isDead())
    {
      entity.destroy();
    }
  }
};