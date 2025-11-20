//-----------------------------------------------------------------------------------
// file:    StateMachineSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   System that updates all entities with StateMachine components.
//          Calls the state machine's update method which handles state lifecycle
//          (on_enter, on_update, on_exit) and deferred state transitions.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"

namespace Engine
{
  namespace Systems
  {
    class StateMachineSystem : public Engine::ECS::System {
    public:
      StateMachineSystem() {
        require_components<Components::StateMachine>();
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* state_machine = entity.get_component<Components::StateMachine>();
        state_machine->update(entity);
      }
    };

  } // namespace Systems
} // namespace Engine
