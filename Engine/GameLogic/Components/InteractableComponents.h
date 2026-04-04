//-----------------------------------------------------------------------------------
// file:    InteractableComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for interactable objects. Includes
//          Interactable component for handling proximity-based interactions
//          with players. Provides on_enter, on_update, and on_exit callbacks
//          when player enters/stays/exits the interaction radius.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

namespace Components
{
  /// Component for proximity-based interactions with players.
  /// When a player entity enters the interaction radius, on_enter is called.
  /// While the player remains in range, on_update is called each frame.
  /// When the player exits the radius, on_exit is called.
  struct Interactable
  {
    // Interaction radius (world units)
    f32 radius = 100.0f;

    // Function names for FunctionRegistry lookup (serializable)
    // Signature: void(Engine::ECS::Entity interactable, Engine::ECS::Entity player)
    Engine::SerializableFunction on_enter;
    Engine::SerializableFunction on_update;
    Engine::SerializableFunction on_exit;

    // Internal state tracking (not serialized to file)
    bool player_in_range = false;
    Engine::ECS::EntityID last_player_id = Engine::ECS::INVALID_ENTITY;

    Interactable() = default;

    Interactable(f32 interaction_radius,
      const std::string& enter_func = "",
      const std::string& update_func = "",
      const std::string& exit_func = "")
      : radius(interaction_radius)
      , on_enter(enter_func)
      , on_update(update_func)
      , on_exit(exit_func)
      , player_in_range(false)
      , last_player_id(Engine::ECS::INVALID_ENTITY)
    {
    }

    // Reset the interaction state (useful when entity is re-enabled or player changes)
    void reset_state() {
      player_in_range = false;
      last_player_id = Engine::ECS::INVALID_ENTITY;
    }
  };
}

REGISTER_COMPONENT(Components::Interactable, radius, on_enter, on_update, on_exit)

