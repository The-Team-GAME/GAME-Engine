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
#include "CollisionSystem.h"
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

phy2D::Physics2DSystem() {
  require_components<Components::Transform2D, Components::RigidBody, Components::BoxCollider>();
}

void phy2D::update()
{
  // so here we update all the kinematics and stuff first
  for (ECS::EntityID entity_id : entities) {
    ECS::Entity entity(entity_id);
    update_entity(entity);
  }
  // now we check for collision and resolve them here? idk LOL
  for (ECS::EntityID entity_id : entities)
  {
    ECS::Entity entity(entity_id);
    Components::BoxCollider* box = entity.get_component<Components::BoxCollider>();
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    for (ECS::EntityID entity_id2 : entities)
    {
      if (entity_id == entity_id2)
      {
        continue;
      }
      ECS::Entity other(entity_id2);
      Components::BoxCollider* other_box = other.get_component<Components::BoxCollider>();
      Components::RigidBody* other_rigid = other.get_component<Components::RigidBody>();
      if (col2D::rect_rect_coll(*box, *other_box)) //captain, it seems we have hit something
      {
        // this gona be the most rabak collision resolution ever
        std::cout << "TRUE " << box->detect_collisions << " " << other_box->detect_collisions << std::endl;
        if (box->detect_collisions == true && other_box->detect_collisions == false)
        {
          std::cout << "ONE\n";
          // other_box is the static one
          Components::Transform2D* xform = entity.get_component<Components::Transform2D>();
          AABB minkowski = col2D::minkowski_AABB(*box, *other_box);
          Vec2D pen_vec = col2D::penetrationVector(minkowski);
          // some magic right here. i understand it rn but maybe alcohol helps

          rigid->velocity -= pen_vec;
          rigid->next_pos += pen_vec * Time::get_fixed_dt() * 2;
          rigid->velocity.y = 0.f;
          update_transform(*rigid, *xform);
        }
        else if (box->detect_collisions == false && other_box->detect_collisions == true)
        {
          std::cout << "TWO\n";
          // box is the static one
          Components::Transform2D* other_xform = other.get_component<Components::Transform2D>();
          AABB minkowski = col2D::minkowski_AABB(*other_box, *box);
          Vec2D pen_vec = col2D::penetrationVector(minkowski);
          other_rigid->velocity -= pen_vec;
          other_rigid->next_pos += pen_vec * Time::get_fixed_dt() * 2;
          other_rigid->velocity.y = 0.f;
          update_transform(*other_rigid, *other_xform);
        }
        // if both detect_collision is false then no point doing LOL
        // we wont do elastic collisions yet, cuz its not needed so i skip
      }
    }
  }
}
void phy2D::update_entity(Engine::ECS::Entity& entity){
  Components::Transform2D* xform = entity.get_component<Components::Transform2D>();
  Components::RigidBody* rigid    = entity.get_component<Components::RigidBody>();
  Components::BoxCollider* box    = entity.get_component<Components::BoxCollider>();
  //if (rigid->apply_physics) // this one is kinda wrong since its only the force part but the objects need to update based in their velocities too
  //{
    update_kinematics(*rigid);
    update_transform(*rigid, *xform);
    col2D::update_AABB(*rigid, *box);
    //std::cout << "velocity: " << rigid->velocity << " || position: " << rigid->curr_pos << std::endl;
  //}
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
  // acceleration will contain a value if the entity is affected by gravity
  Vec2D acc = rigid.force / rigid.mass + rigid.acceleration;
  
  // V = a x dt => Velocity equals acceleration x delta time
  Vec2D prev_velocity = rigid.velocity * rigid.friction;
  rigid.velocity = prev_velocity + acc * Time::get_fixed_dt();

  // Checking terminal velocity here
  // using the magnitude squared to check cuz its cheaper than sqrt-ing the value
  if (rigid.velocity.get_magnitude_sq() > pow(rigid.terminal_velocity, 2))
  {
    rigid.velocity = Math::normalise_vector(rigid.velocity) * rigid.terminal_velocity;
  }
  // setting the next position
  rigid.next_pos = rigid.curr_pos + rigid.velocity * Time::get_fixed_dt();
}

void phy2D::update_transform(Components::RigidBody& rigid, Components::Transform2D& xform)
{

  // Then we set the current position as next_pos
  rigid.curr_pos = rigid.next_pos;

  // Cuz for physics im using bottom left as 0,0 
  // but graphics uses top right, theres some magic to do
  xform.position = Vec2D(rigid.curr_pos.x, rigid.curr_pos.y);
}

// Maybe we check for collision here and then do translate as the last step?