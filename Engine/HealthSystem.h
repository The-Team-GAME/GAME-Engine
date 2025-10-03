//-----------------------------------------------------------------------------------
// file:    HealthSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the HealthSystem which manages entity health and
//          handles entity destruction when health reaches zero. Automatically
//          destroys entities with depleted health.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
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

    if (health->is_dead())
    {
      entity.destroy();
    }
  }
};