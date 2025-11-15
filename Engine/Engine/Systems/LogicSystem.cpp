#include "LogicSystem.h"
#include "../Math/Vector2D.h"
#include "../InputManager.h"
#include "PhysicsSystem.h"
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
  Vec2D patrol_speed(300.f, 0.f);
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
  Components::Transform2D*  xform = entity.get_component<Components::Transform2D>();
  Components::RigidBody*    rigid = entity.get_component<Components::RigidBody>();
  Components::Logic*        logic = entity.get_component<Components::Logic>();
  switch (logic->state)
  {
  case entity_state::IDLE:
    rigid->velocity.x = 0.f;
    // Animation here
    break;
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

  // PLAYER SPECIFIC STATES
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

  // ENEMY SPECIFIC STATES
  case entity_state::PATROL:
    // Condition to swap into CHASE state here

    // Patrol behaviour
    if (xform->position.x >= logic->right_bound) // Moved past right bound
    {
      xform->position.x = logic->right_bound;
      logic->direction = -1.f;
    }
    else if (xform->position.x <= logic->left_bound) // Moved past left bound
    {
      xform->position.x = logic->left_bound;
      logic->direction = 1.f;
    }
    rigid->velocity = patrol_speed * logic->direction;
    break;
  case entity_state::CHASE:
    // Condition to swap into PATROL state here

    // Chase behaviour
    break;
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




