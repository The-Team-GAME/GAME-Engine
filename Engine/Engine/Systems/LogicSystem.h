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
  PATROL,
  CHASE,
};

namespace Components
{
  struct Logic
  {
    entity_state state;

    Logic(entity_state starting_state) : state(starting_state) {};
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



