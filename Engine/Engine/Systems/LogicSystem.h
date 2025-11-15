#pragma once
#include "../ECS/ECS.h"
#include "../Components/Components.h"

enum entity_state {
  IDLE,
  ATTACK,
  STUN,

  // PLAYER SPECIFIC STATES
  MOVING_LEFT,
  MOVING_RIGHT,
  JUMPING,

  // ENEMY SPECIFIC STATES
  PATROL, // Move left to right, within given boundary
  CHASE,  // Chase the player within given boundary
};

namespace Components
{
  struct Logic
  {
    entity_state state;
    // For NPCs
    f32 direction = 1.f;
    f32 left_bound;
    f32 right_bound;
    // The patrol limit, pls set it a little under the length of the platform
    


    // Player ctor
    Logic(entity_state starting_state) : state(starting_state) {};

    // Enemy ctor
    Logic(entity_state starting_state, f32 min_bound, f32 max_bound): 
      state(starting_state), 
      left_bound(min_bound), 
      right_bound(max_bound)
    {
      //Empty by default
    };
  };
}

namespace Engine {
  namespace Systems
  {
    class LogicSystem : public Engine::ECS::System
    {
    public:
      // Might need to include animation stuff here too
      LogicSystem();

      static void load_values();

      void update_entity(Engine::ECS::Entity& entity) override;

      static void take_input(ECS::Entity player);

      void run_state(ECS::Entity entity);

      void move_left(Components::RigidBody& rigid);

      void move_right(Components::RigidBody& rigid);

      void jump(Components::RigidBody& rigid);
    };
  }
}



