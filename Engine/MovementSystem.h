//-----------------------------------------------------------------------------------
// file:    MovementSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a system that handles entity movement by updating
//          positions based on velocity and delta time in the ECS framework.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"

class MovementSystem : public ECS::System {
public:
  MovementSystem() {
    require_components<Components::Position, Components::Velocity>();
  }

  /*=================================================================================
  Function Brief:
  Updates a single entity's position based on its velocity and the fixed delta time.
  This function is called automatically by the ECS framework for each entity that
  has the required components.

  Parameters:
  entity - Reference to the entity being updated.

  Where to call it:
  This is called automatically by the ECS system during the update loop. Do not
  call this directly.
  ==================================================================================*/
  void update_entity(ECS::Entity& entity) override {
    auto* pos = entity.get_component<Components::Position>();
    auto* vel = entity.get_component<Components::Velocity>();

    pos->X() += vel->X() * Engine::Time::get_fixed_dt();
    pos->Y() += vel->Y() * Engine::Time::get_fixed_dt();
  }
};