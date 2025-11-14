#pragma once
#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Math/Vector2D.h"
namespace { // in the cpp file
  // Values to be loaded from file

  /*===============
  / Damage Values /
  ===============*/
  f32 player_dmg;         // How much damage the player deals
  f32 enemy_dmg;          // How much damage the enemy deals

  /*==================
  / Knockback Values /
  ==================*/
  Vec2D player_knockback; // How much the player gets knockbacked
  Vec2D enemy_knockback;  // How much the enemy gets knockbacked
}
namespace Engine {
  namespace Systems
  {
    class LogicSystem : public Engine::ECS::System
    {
      // Might need to include animation stuff here too
      LogicSystem()
      {
        require_components<Components::RigidBody>();
      }

      static void load_values();

#pragma region Callbacks 
      /*==================
      / Callback Section /
      ==================*/

      // Damage to give to the recieving party
      static void player_atk() //put in the enemy's health component here
      {
        // No Physics Portion
      }

      // Damage to give to the recieving party
      static void enemy_atk()
      {
        // No Physics Portion
      }

      static void recieve_atk(ECS::Entity& entity)
      {
        // No Physics Portion
      }
#pragma endregion
    };

  }
}

