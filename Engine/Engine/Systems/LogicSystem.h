#pragma once
#include "../ECS/ECS.h"
#include "../Components/Components.h"

enum entity_state {
  ATTACK,
  STUN,

  // PLAYER SPECIFIC STATES
  IDLE,
  MOVING_LEFT,
  MOVING_RIGHT,
  JUMPING,

  // ENEMY SPECIFIC STATES
  PATROL, // Move left to right, within given boundary
  CHASE,  // Chase the player within given boundary
};

namespace Components
{
   struct Player_
   {
     // Exists just to identify the player
   };
  struct Logic
  {
    entity_state state;
    // For NPCs
    f32 pause_timer = 1.5f;
    f32 direction = 1.f;
    f32 sight_distance = 250.f;
    f32 left_patrol_bound;
    f32 right_patrol_bound;
    f32 left_chase_bound;
    f32 right_chase_bound;
    // The patrol limit, pls set it a little under the length of the platform



    // Player ctor
    Logic(entity_state starting_state) : state(starting_state) {};

    // Enemy ctor
    Logic(entity_state starting_state, f32 left_chase_bound, f32 left_patrol_bound, f32 right_patrol_bound, f32 right_chase_bound) :
      state(starting_state),
      left_patrol_bound(left_patrol_bound), right_patrol_bound(right_patrol_bound),
      left_chase_bound(left_chase_bound), right_chase_bound(right_chase_bound)
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



