#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"

class HealthSystem : public ECS::System
{
public:
  HealthSystem()
  {
    require_components<Components::Health>();
  }

  void update_entity(ECS::Entity& entity) override
  {
    auto health = entity.get_component<Components::Health>();

    if (health->isDead())
    {
      entity.destroy();
    }
  }
};