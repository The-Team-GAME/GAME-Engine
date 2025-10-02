

//-----------------------------------------------------------------------------------
// file:    SimplePlayerMovementSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a basic WASD keyboard-controlled movement system
//          for entities with SimplePlayerControl components in the ECS framework.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"
#include "Core.h"

class SimplePlayerMovementSystem : public ECS::System {
public:
  /*=================================================================================
  Function Brief:
  Constructs the SimplePlayerMovementSystem and specifies that it requires entities
  to have both Position and SimplePlayerControl components.
  ==================================================================================*/
  SimplePlayerMovementSystem() {
    require_components<Components::Position, Components::SimplePlayerControl>();
  }

  /*=================================================================================
  Function Brief:
  Updates entity position based on WASD keyboard input and the control component's
  speed setting. Called automatically by the ECS framework for each entity with
  the required components.

  Parameters:
  entity - Reference to the entity being updated.
  ==================================================================================*/
  void update_entity(ECS::Entity& entity) override {
    auto* pos = entity.get_component<Components::Position>();
    auto* control = entity.get_component<Components::SimplePlayerControl>();

    if (Engine::Input::IsKeyHeld(GLFW_KEY_W)) {
      pos->Y() -= control->speed * Engine::Time::get_fixed_dt();
    }
    if (Engine::Input::IsKeyHeld(GLFW_KEY_S)) {
      pos->Y() += control->speed * Engine::Time::get_fixed_dt();
    }
    if (Engine::Input::IsKeyHeld(GLFW_KEY_A)) {
      pos->X() -= control->speed * Engine::Time::get_fixed_dt();
    }
    if (Engine::Input::IsKeyHeld(GLFW_KEY_D)) {
      pos->X() += control->speed * Engine::Time::get_fixed_dt();
    }
  }
};

