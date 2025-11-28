#include "LogicSystem.h"
#include "../Math/Vector2D.h"
#include "../InputManager.h"
#include "PhysicsSystem.h"
#include "time.h"
using namespace Engine;
using namespace Systems;
using logi = LogicSystem;
using phy2D = Physics2DSystem;

namespace { // in the cpp file
  // Values to be loaded from file
  /*===============/
  / Movement stuff /
  /===============*/
  f32 walking_x_vel = 400.f;
  Vec2D patrol_speed(150.f, 0.f);
  Vec2D jumping_force(0.f, -3500000.f);

  /*==================/
  / Enemy Logic stuff /
  /==================*/

  //TODO after m3: maybe add logic so that they cant walk past their platform
}

// Might need to include animation stuff here too
logi::LogicSystem()
{
  require_components<Components::Transform2D, Components::RigidBody, Components::Logic>();
}

void logi::load_values()
{
  // Fill up soon
}

void logi::update_entity(Engine::ECS::Entity& entity)
{
  run_state(entity);

}

void logi::take_input(ECS::Entity player)
{
  Components::RigidBody* player_rigid = player.get_component<Components::RigidBody>();
  Components::Logic* player_logic = player.get_component<Components::Logic>();

  // resettng the state
  player_logic->state = IDLE;
  // put this first because we need to reset the flag if possible
  if (Input::is_released(KeyCode::Space) && player_rigid->is_jumping)
  {
    player_rigid->is_jumping = false;
  }

  if (Input::is_held(KeyCode::Space)
    && player_rigid->on_ground
    && !player_rigid->is_jumping)
  {
    player_logic->state = JUMPING;
    player_rigid->is_jumping = true;
  }
  else if (Input::is_pressed(KeyCode::A) || Input::is_held(KeyCode::A))
  {
    player_logic->state = MOVING_LEFT;
  }
  else if (Input::is_held(KeyCode::D))
  {
    player_logic->state = MOVING_RIGHT;
  }
}

void logi::run_state(ECS::Entity entity)
{
  // getting the player for the NPC state machine logic
  ECS::Entity player = ECS::get_first_entity_with<Components::Player_>();
  Components::BoxCollider*  player_box = player.get_component<Components::BoxCollider>();

  Components::Transform2D*  xform = entity.get_component<Components::Transform2D>();
  Components::RigidBody*    rigid = entity.get_component<Components::RigidBody>();
  Components::Logic*        logic = entity.get_component<Components::Logic>();

  // We creating the LOS of the NPC
  // 200 is a magic number, i just need the LOS to have a thickness for now, but the LOS needs to be tall enough to still see the player if they jump
  AABB line_of_sight(logic->sight_distance, 200.f);
  // This sets the position of the LOS wrt the direction the NPC is facing
  line_of_sight.pos = xform->position + logic->sight_distance / 2 * logic->direction;
  switch (logic->state)
  {
#pragma region shared states

  case entity_state::ATTACK:
    // Motion lock here so cannot move with manual input
    // 
    // Callback here (if have)
    // Animation here
    break;
  case entity_state::STUN:
    // Physics motion lock here
    // Animation here
    break;
#pragma endregion

#pragma region player states
  // PLAYER SPECIFIC STATES
  case entity_state::IDLE:
    rigid->velocity.x = 0.f;
    // Animation here
    break;
  case entity_state::MOVING_LEFT:
    // Physics Movement code here
    move_left(*rigid);
    break;
  case entity_state::MOVING_RIGHT:
    // Physics Movement code here
    move_right(*rigid);
    break;
  case entity_state::JUMPING:
    // Physics Movement code here
    jump(*rigid);
    break;
#pragma endregion
#pragma region enemy states
  // ENEMY SPECIFIC STATES
  case entity_state::PATROL:
    // Condition to swap into CHASE state here
    // Basically we want to draw a straight line infront of the NPC and see if the player's hitbox collides with it

    if (line_of_sight.intersects(player_box->aabb))
    {
      logic->state = CHASE;
      break;
    }

    // Patrol behaviour
    if (xform->position.x > logic->right_patrol_bound && logic->direction == 1.f) // Moved past right bound
    {
      logic->direction = -1.f;
      logic->pause_timer = 1.f; // Pause
      rigid->velocity.x = 0.f;
    }
    else if (xform->position.x < logic->left_patrol_bound && logic->direction == -1.f) // Moved past left bound
    {
      logic->direction = 1.f;
      logic->pause_timer = 1.f;
      rigid->velocity.x = 0.f;
    }
    else if (logic->pause_timer > 0.f)
    {
      // dosent really matter if we overshot
      logic->pause_timer -= Time::get_fixed_dt();
    }
    else
    {
      rigid->velocity = patrol_speed * logic->direction;
    }
    break;

  case entity_state::CHASE:
    // Condition to swap into Attack state here
    
    // Condition to swap into PATROL state here
    // We will make the NPCs swap back to patrol mode if they exceed their chasing bounds
    // For now i want to make it so that they simply are confined to their patrol area, in the future maybe let them chase as long as they are on the same platform and maintains line of sight with the player
    if (xform->position.x > logic->right_chase_bound || xform->position.x < logic->left_chase_bound)
    {
      logic->state = PATROL;
      break;
    }
    else if (!line_of_sight.intersects(player_box->aabb))
    {
      logic->state = PATROL;
      break;
    }
    
    // Chase behaviour
    rigid->velocity = patrol_speed * logic->direction;
    break;
#pragma endregion
  }
}

void logi::move_left(Components::RigidBody& rigid)
{
  rigid.velocity.x = -walking_x_vel;
}

void logi::move_right(Components::RigidBody& rigid)
{
  rigid.velocity.x = walking_x_vel;
}

void logi::jump(Components::RigidBody& rigid)
{
  rigid.is_jumping = true;
  rigid.on_ground = false;
  phy2D::add_force(rigid, jumping_force);
}




