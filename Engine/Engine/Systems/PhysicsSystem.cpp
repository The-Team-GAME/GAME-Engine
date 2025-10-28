//-----------------------------------------------------------------------------------
// file:    RotationSystem.h
// project: Monkey Stole My GPA
//
// author:   (100%)
//          Primary author
//
// email:   @digipen.edu
//
// brief:   This file defines a system that handles entity rotation by updating
//          rotation angles based on rotation speed and delta time in the ECS
//          framework.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "PhysicsSystem.h";

using namespace Engine;
using namespace Systems;
using phy2D = Physics2DSystem;

phy2D::Physics2DSystem() {
  require_components<Components::RigidBody, Components::BoxCollider>();
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
// i leave this here first
/*void update_entity(Engine::ECS::Entity& entity) override {
  auto* transform = entity.get_component<Components::Transform2D>();
  auto* rigid = entity.get_component<Components::RigidBody>();

  transform->rotation += rigid->angular_velocity * Engine::Time::get_fixed_dt();

  transform->position.x += rigid->linear_velocity.x * Engine::Time::get_fixed_dt();
  transform->position.y += rigid->linear_velocity.y * Engine::Time::get_fixed_dt();
}*/

void phy2D::update_kinematics(Components::RigidBody& rigid)
{
  // F = m x a => Force equals mass x acceleration
  rigid.acceleration = rigid.force / rigid.mass;
  // V = a x dt => Velocity equals acceleration x delta time
  rigid.velocity = rigid.acceleration * Time::get_fixed_dt();

  // Checking terminal velocity here
  if (rigid.velocity.get_magnitude_sq() > pow(rigid.terminal_velocity, 2))
  {
    rigid.velocity = Math::normalise_vector(rigid.velocity) * rigid.terminal_velocity;
  }
}

void phy2D::update_transform(Components::RigidBody& rigid)
{
  rigid.next_pos = rigid.curr_pos + rigid.velocity * Time::get_fixed_dt();
  // now we have the supposed next position.
  // but first we need to check if anything collides

  // insert collision function here
  // the collision function should update next_pos

  // Then we set the current position after everything is done
  rigid.curr_pos = rigid.next_pos;
}

// Maybe we check for collision here and then do translate as the last step?