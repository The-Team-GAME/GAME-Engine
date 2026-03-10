//-----------------------------------------------------------------------------------
// file:    PunchingBagStateStateCallbacks.h
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
using namespace Engine;

namespace PunchingBagStateCallbacks
{
  inline void idle_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
    Log::info("Punching Bag: Entered Idle state");
  }

  inline void left_punch_on_update(ECS::Entity entity)
  {
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(animation);
    if (animation->has_ended())
    {
      Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
      assert(sm);
      sm->change_state("Idle");
    }
  }

  inline void right_punch_on_update(ECS::Entity entity)
  {
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(animation);
    if (animation->has_ended())
    {
      Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
      assert(sm);
      sm->change_state("Idle");
    }
  }
}

REGISTER_STATE_MACHINE_BLUEPRINT(PunchingBagState, "Idle",
  STATE("Idle", PunchingBagStateCallbacks::idle_on_enter, NO_CALLBACK, NO_CALLBACK)
  STATE("Left", NO_CALLBACK, PunchingBagStateCallbacks::left_punch_on_update, NO_CALLBACK)
  STATE("Right", NO_CALLBACK, PunchingBagStateCallbacks::right_punch_on_update, NO_CALLBACK)
)