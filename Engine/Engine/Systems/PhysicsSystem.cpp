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

  rigid->on_ground = false;
  phy2D::update_kinematics(*rigid, *xform);
  rigid->force.zero();
  //std::cout << "velocity: " << rigid->velocity << " || position: " << rigid->curr_pos << std::endl;
}

void phy2D::update_kinematics(Components::RigidBody& rigid, Components::Transform2D& xform)
{
  Vec2D next_pos;
  // F = m x a => Force equals mass x acceleration
  // acceleration will contain a value if the entity is affected by gravity
  rigid.acceleration = rigid.force / rigid.mass + rigid.gravity_acc;
  // V = a x dt => Velocity equals acceleration x delta time
  // i increased friction for the x axis to test cuz we not sure if we want to do it like this
  Vec2D prev_velocity = Vec2D(rigid.velocity.x * 0.80f, rigid.velocity.y * rigid.friction);
  rigid.velocity = prev_velocity + rigid.acceleration * Time::get_fixed_dt();

  // Checking terminal velocity here
  // using the magnitude squared to check cuz its cheaper than sqrt-ing the value
  if (rigid.velocity.get_magnitude_sq() > pow(rigid.terminal_velocity, 2))
  {
    rigid.velocity = Math::normalise_vector(rigid.velocity) * rigid.terminal_velocity;
  }
  // setting the next position
  next_pos = xform.position + rigid.velocity * Time::get_fixed_dt();

  // Update transform after the new position has been calculated
  xform.position = next_pos;
  xform.rotation += rigid.angular_velocity * Engine::Time::get_fixed_dt();
}

void phy2D::kill_normal_component(Components::RigidBody& rigid, Vec2D mvt)
{
  Vec2D normalised_mtv = normalise_vector(mvt);
  f32 magnitude_of_normal_force = dot_product(rigid.velocity, normalised_mtv);
  rigid.velocity -= normalised_mtv * magnitude_of_normal_force;
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

void phy2D::walk_left(Components::RigidBody& rigid)
{
  rigid.velocity.x = -200.f;
}

void phy2D::walk_right(Components::RigidBody& rigid)
{
  rigid.velocity.x = 200.f;
}

void phy2D::jump(Components::RigidBody& rigid)
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