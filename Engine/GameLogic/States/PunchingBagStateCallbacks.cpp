//-----------------------------------------------------------------------------------
// file:    PlayerStateCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
// 
// email:   c.mingxian@digipen.edu
//
// brief:   Punching Bag state machine callback implementations (serializable)
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "../Components/GameplayComponent.h"
using namespace Engine;

namespace PunchingBagStateCallbacks
{
  inline void idle_on_update([[maybe_unused]] Engine::ECS::Entity entity) {
    // Empty as designed
  }

  inline void hit_on_enter(Engine::ECS::Entity entity) 
  {
    // Check which direction the punching bag is being hit from
    ECS::Entity player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    Components::Transform2D* player_xform = player_entity.get_component<Components::Transform2D>();

    Components::Transform2D* bag_xform = entity.get_component<Components::Transform2D>();
  }

  inline void left_on_update([[maybe_unused]] Engine::ECS::Entity entity) {
    // Empty as designed
  }

  inline void right_on_update([[maybe_unused]] Engine::ECS::Entity entity) {
    // Empty as designed
  }
}
REGISTER_STATE_MACHINE_BLUEPRINT(EmptyState, "Idle",
  STATE("Idle", NO_CALLBACK, PunchingBagStateCallbacks::idle_on_update, NO_CALLBACK)
  STATE("Hit", NO_CALLBACK, NO_CALLBACK, NO_CALLBACK)
  STATE("Left", NO_CALLBACK, NO_CALLBACK, NO_CALLBACK)
  STATE("Right", NO_CALLBACK, NO_CALLBACK, NO_CALLBACK)
)
