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
#include "../Components/EnemyHealthComponent.h"
using namespace Engine;

namespace PunchingBagStateCallbacks
{
  inline void idle_on_update([[maybe_unused]] Engine::ECS::Entity entity) 
  {
    // Empty as designed
  }

  inline void hit_on_enter(Engine::ECS::Entity entity) 
  {
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
    assert(rigid && sm);

    if (rigid->force.x > 0.f) // right
    {
      sm->change_state("Right");
    }
    else
    {
      sm->change_state("Left");
    }
  }

  inline void left_on_update(Engine::ECS::Entity entity) 
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

  inline void right_on_update(Engine::ECS::Entity entity)
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

  inline void death_on_enter(Engine::ECS::Entity entity)
  {
    Components::EnemyHealth* hp = entity.get_component<Components::EnemyHealth>();
    assert(hp);
    hp->heal(hp->max_hp);

    Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
    sm->change_state("Idle");
  }
};

REGISTER_STATE_MACHINE_BLUEPRINT(PunchingBag, "Idle",
  STATE("Idle", NO_CALLBACK, PunchingBagStateCallbacks::idle_on_update, NO_CALLBACK)
  STATE("Hit", PunchingBagStateCallbacks::hit_on_enter, NO_CALLBACK, NO_CALLBACK)
  STATE("Left", NO_CALLBACK, PunchingBagStateCallbacks::left_on_update, NO_CALLBACK)
  STATE("Right", NO_CALLBACK, PunchingBagStateCallbacks::right_on_update, NO_CALLBACK)
  STATE("Death", PunchingBagStateCallbacks::death_on_enter, NO_CALLBACK, NO_CALLBACK)
)
