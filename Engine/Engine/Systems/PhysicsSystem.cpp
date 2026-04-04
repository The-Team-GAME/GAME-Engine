//-----------------------------------------------------------------------------------
// file:    PhysicsSystem.cpp
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file defines a system that handles the physics updates for all
//          entities tht contain rigidbody and transform2d components.
//          Entities will have their rigidbody updated which in turn updates the
//          transform2d component as well.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "../Perf.h"
#include "CollisionSystem.h"
#include "PhysicsSystem.h"
#include "../Math/Math.h"
#include "../Logger.h"
using namespace Engine;
using namespace Systems;
using phy2D = Physics2DSystem;
using col2D = Collision2DSystem;
/*===================================/
/ DISCLAIMER!!!                      /
/ 0,0 is the top right of the screen /
/ Meaning that you need to           /
/ +y to GO DOWN and -y to GO UP      /
/===================================*/

// eventually we'll deserialise these values from file bah
namespace {
  // Basically this value is how seep a slope can be before its no longer considered walkable/ on the ground
  constexpr f32 slope_angle_threshold = 60.0f;
  const f32 slope_radian_threshold = slope_angle_threshold * static_cast<f32>(std::numbers::pi / 180.0f);
}

f32 phy2D::get_slope_angle_threshold()
{
  return slope_angle_threshold;
}

f32 phy2D::get_slope_radian_threshold()
{
  return slope_radian_threshold;
}

phy2D::Physics2DSystem() {
  require_components<Components::Transform2D, Components::RigidBody>();
} // Components::BoxCollider

void phy2D::update()
{
  Engine::Perf::start("Physics Update");
  // Basically right, we cant reset kinematic values here because the game state update() runs before this
  // Anything that we want to reset needs to be done in the gamestate, but most of it is player stuff so its fine
  // so here we update all the kinematics and stuff first
  for (ECS::EntityID entity_id : entities) {
    ECS::Entity entity(entity_id);
    update_entity(entity);
  }
  Engine::Perf::stop("Physics Update");
  // collision check afterwards
}

void phy2D::update_entity(ECS::Entity entity) 
{
  Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
  //Components::Transform2D* xform = entity.get_component<Components::Transform2D>();

  // We account for the gravitational forces here so that the update_kinematics function can be used for post physics as well
  if (rigid->use_gravity)
  {
    rigid->force += rigid->gravity * rigid->mass;
    //std::cout << "Gravity Force: " << gravity_force << std::endl;
  }

  // Moved update kinematics and update position to post physics so that all the forces are applied at once
  //Log::info("velocity: ({}, {}) || position: ({}, {})", rigid->velocity.x, rigid->velocity.y, rigid->curr_pos.x, rigid->curr_pos.y);
}

void phy2D::update_kinematics( Components::RigidBody& rigid )
{
  if (rigid.apply_force) 
  {
    rigid.acceleration = rigid.force / rigid.mass;
    rigid.velocity += rigid.acceleration * Time::get_fixed_dt();
  }
  // Checking terminal velocity here
  // using the magnitude squared to check cuz its cheaper than sqrt-ing the value
  /*if (rigid.velocity.get_magnitude_sq() > pow(rigid.terminal_velocity, 2))
  {
    rigid.velocity = Math::normalise_vector(rigid.velocity) * rigid.terminal_velocity;
  }*/
  if (abs(rigid.velocity.x) > rigid.x_terminal_velocity)
  {
    rigid.velocity.x = rigid.velocity.x > 0 ? 
      rigid.x_terminal_velocity : -rigid.x_terminal_velocity;
  }
  if (abs(rigid.velocity.y) > rigid.y_terminal_velocity)
  {
    rigid.velocity.y = rigid.velocity.y > 0 ?
      rigid.y_terminal_velocity : -rigid.y_terminal_velocity;
  }
}

void phy2D::update_position(
  Components::RigidBody& rigid,
  Components::Transform2D& xform)
{
  // setting the next position
  Vec2D next_pos = xform.position + rigid.velocity * Time::get_fixed_dt();

  // Update transform after the new position has been calculated
  xform.position = next_pos;
  xform.rotation += rigid.angular_velocity * Engine::Time::get_fixed_dt();
}

void phy2D::zero_normal_velocity(
  Components::RigidBody& rigid,
  CollisionResult const& collision)
{
  Vec2D normalised_mtv = normalise_vector(collision.mtv);
  f32 normal_velocity_magnitude = dot_product(rigid.velocity, normalised_mtv);
  rigid.velocity -= normalised_mtv * normal_velocity_magnitude;
}

void phy2D::add_force(Components::RigidBody& rigid, Vec2D force)
{
  rigid.force += force;
}

