//-----------------------------------------------------------------------------------
// file:    RotationSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a system that handles entity rotation by updating
//          rotation angles based on rotation speed and delta time in the ECS
//          framework.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"
#include "Core.h"

class RotationSystem : public ECS::System {
public:
  /*=================================================================================
  Function Brief:
  Constructs the RotationSystem and specifies that it requires entities to have
  both Rotation and RotationSpeed components.
  ==================================================================================*/
  RotationSystem() {
    require_components<Components::Transform2D, Components::RigidBody>();
  }

  /*=================================================================================
  Function Brief:
  Updates a single entity's rotation angle based on its rotation speed and the
  fixed delta time. This function is called automatically by the ECS framework
  for each entity that has the required components.

  Parameters:
  entity - Reference to the entity being updated.
  ==================================================================================*/
  void update_entity(ECS::Entity& entity) override {
    auto* transform = entity.get_component<Components::Transform2D>();
    auto* rigid = entity.get_component<Components::RigidBody>();

    transform->rotation += rigid->angular_velocity * Engine::Time::get_fixed_dt();
  }
};

