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
  Vec2D knockback_force{ -1000000.f, 50000.f };
  const Vec2D gravity_acc = { 0.f, 1000.f };
  const f32 STATIC_FRICTION_THRESHOLD = 20.0f;

  const f32 slope_angle_limit = 60.f; // This is basically the limit of what will count as a slope
  /*
        /
       /
      /_____ (60 degree slope for example)
  */
}

f32 phy2D::get_slope_angle_limit()
{
  return slope_angle_limit;
}

Vec2D phy2D::get_gravity()
{
  return gravity_acc;
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

void phy2D::update_entity(ECS::Entity& entity) {
  Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
  Components::Transform2D* xform = entity.get_component<Components::Transform2D>();
  Components::BoxCollider* box = entity.get_component<Components::BoxCollider>();
  phy2D::update_kinematics(*rigid, *box, *xform);
  rigid->force.zero();
  //Log::info("velocity: ({}, {}) || position: ({}, {})", rigid->velocity.x, rigid->velocity.y, rigid->curr_pos.x, rigid->curr_pos.y);
}

void phy2D::update_kinematics(
  Components::RigidBody& rigid, 
  Components::BoxCollider& box, 
  Components::Transform2D& xform)
{
  // F = m x a => Force equals mass x acceleration
  // acceleration will contain a value if the entity is affected by gravity
  rigid.acceleration = rigid.force / rigid.mass;
  if (rigid.use_gravity)
  {
    rigid.acceleration += gravity_acc;
  }
  
  // From now on we will apply friction on collision like how its supposed to be
  // There wont be any air resistance so that the player can be controlled more accurately
  rigid.velocity.x *= box.kinetic_friction;
  rigid.velocity += rigid.acceleration * Time::get_fixed_dt();

  // Checking terminal velocity here
  // using the magnitude squared to check cuz its cheaper than sqrt-ing the value
  if (rigid.velocity.get_magnitude_sq() > pow(rigid.terminal_velocity, 2))
  {
    rigid.velocity = Math::normalise_vector(rigid.velocity) * rigid.terminal_velocity;
  }
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

void phy2D::apply_surface_friction(
  Components::RigidBody& rigid,
  CollisionResult const& collision,
  [[maybe_unused]] f32 static_friction,
  f32 kinetic_friction)
{
  // At this point, the velocity of the entity should already be parallel to the surface its colliding with since zero_normal_velocity was called prior to this function.
  Vec2D normal = normalise_vector(collision.mtv);
  Vec2D tangent = normalise_vector(collision.get_tangent());
  if (angle_of_slope(tangent) > phy2D::get_slope_angle_limit())
  {
    return;
  }
  // Get velocity component parallel to the surface
  f32 tangent_speed = dot_product(rigid.velocity, tangent);
  f32 speed = fabsf(tangent_speed);

  if (speed < STATIC_FRICTION_THRESHOLD)
  {
    // Entity is moving very slowly - apply static friction
    // Remove the tangent velocity component completely
    rigid.velocity -= tangent * tangent_speed;
    return;  // Exit early - entity is now stationary relative to surface
  }

  // Entity is moving fast enough - apply kinetic friction
  // Apply friction force proportional to speed
  // Direction opposes motion along the surface
  Vec2D friction_direction = (tangent_speed < 0) ? (tangent * -1.0f) : tangent;
  f32 friction_magnitude = speed * kinetic_friction;

  // Apply friction as a velocity change
  rigid.velocity -= friction_direction * friction_magnitude;
}

void phy2D::add_force(Components::RigidBody& rigid, Vec2D force)
{
  rigid.force += force;
}

void phy2D::scale_force(Components::RigidBody& rigid, f32 scale)
{
  rigid.force *= scale;
}

void phy2D::zero_force(Components::RigidBody& rigid)
{
  rigid.force.zero();
}

void phy2D::walk_left(Components::RigidBody& rigid) // not used
{
  rigid.velocity.x = -200.f;
}

void phy2D::walk_right(Components::RigidBody& rigid) // not used
{
  rigid.velocity.x = 200.f;
}

void phy2D::jump(Components::RigidBody& rigid) // not used
{
  rigid.on_ground = false;
  phy2D::add_force(rigid, Vec2D(0.f, -3500000.f));
}

//void phy2D::knockback(ECS::Entity& victim, Vec2D force)
//{
//  Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
//  Components::Transform2D* xform = entity.get_component<Components::Transform2D>();
//  if (rigid.direction == LEFT)
//}