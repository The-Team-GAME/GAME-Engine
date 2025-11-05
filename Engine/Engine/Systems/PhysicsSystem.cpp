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
#include "PhysicsSystem.h"
#include "CollisionSystem.h"
#include "../Perf.h"
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
  f32 walking_velocity = 500.f;
  Vec2D jumping_force(0.f, -40000.f);
}
phy2D::Physics2DSystem() {
  require_components<Components::Transform2D, Components::RigidBody>();
} //, Components::BoxCollider

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

  // now we check for collision and resolve them here? idk LOL

  // First pass: reset on_ground and update AABBs
  for (ECS::EntityID entity_id : entities)
  {
    ECS::Entity entity(entity_id);
    Components::BoxCollider* box = entity.get_component<Components::BoxCollider>();
    if (box == nullptr) { continue; }
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    rigid->on_ground = false;
    col2D::update_AABB(*rigid, *box);
  }

  // Second pass: check collisions using for_each_entity_pair
  // Check ALL entities with RigidBody and BoxCollider against each other
  ECS::for_each_entity_pair<Components::RigidBody, Components::BoxCollider>(
    [this](ECS::Entity entity, ECS::Entity other)
    {
      // Both entities are guaranteed to have RigidBody and BoxCollider
      Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
      Components::RigidBody* other_rigid = other.get_component<Components::RigidBody>();
      Components::BoxCollider* box = entity.get_component<Components::BoxCollider>();
      Components::BoxCollider* other_box = other.get_component<Components::BoxCollider>();

      /* START OF BUILDING COLLISION MASKS */

      // Build collision masks for both entities
      ECS::LayerSignature entity_layers = entity.get_layers();
      ECS::LayerSignature entity_collision_mask = entity_layers;
      entity_collision_mask |= rigid->include_layers;
      entity_collision_mask &= ~rigid->exclude_layers;

      ECS::LayerSignature other_layers = other.get_layers();
      ECS::LayerSignature other_collision_mask = other_layers;
      other_collision_mask |= other_rigid->include_layers;
      other_collision_mask &= ~other_rigid->exclude_layers;

      // Check if layers allow collision (bidirectional check)
      // entity must be willing to collide with other's layers AND vice versa
      if ((entity_collision_mask & other_layers).none() || (other_collision_mask & entity_layers).none()) {
        return; // Layer filtering prevents collision
      }

      /* END OF BUILDING COLLISION MASKS */

      // Check AABB collision
      if (!col2D::rect_rect_coll(*box, *other_box)) {
        return; // No collision
      }

      // this gona be the most rabak collision resolution ever
      //std::cout << "TRUE " << box->detect_collisions << " " << other_box->detect_collisions << std::endl;

      // Resolve collision based on detect_collisions flags
      if (box->detect_collisions && !other_box->detect_collisions)
      {
        // other_box is the static one
        Components::Transform2D* xform = entity.get_component<Components::Transform2D>();
        AABB minkowski = col2D::minkowski_AABB(*box, *other_box);
        Vec2D pen_vec = col2D::penetrationVector(minkowski);
        // some magic right here. i understand it rn but maybe alcohol helps

        //std::cout << "Prev pos: " << rigid->next_pos << std::endl;

        rigid->next_pos -= pen_vec;
        if (pen_vec.y == 0) // sideways collision
        {
          rigid->velocity.x = 0.f;
        }
        else // updown collision
        {
          rigid->velocity.y = 0.f;
          if (pen_vec.y > 0)
          {
            rigid->on_ground = true;
          }
        }
        update_transform(*rigid, *xform);
        //std::cout << "New pos:  " << rigid->next_pos << std::endl;
      }
      //else if (box->detect_collisions == false && other_box->detect_collisions == true)
       //{
       //  // box is the static one
       //  Components::Transform2D* other_xform = other.get_component<Components::Transform2D>();
       //  AABB minkowski = col2D::minkowski_AABB(*other_box, *box);
       //  Vec2D pen_vec = col2D::penetrationVector(minkowski);
       //  other_rigid->velocity -= pen_vec;
       //  other_rigid->next_pos += pen_vec * Time::get_fixed_dt();
       //  if (pen_vec.y == 0) // sideways collision
       //  {
       //    other_rigid->velocity.x = 0.f;
       //    if (pen_vec.y < 0)
       //    {
       //      // std::cout << "on ground\n";
       //      other_rigid->on_ground = true;
       //    }
       //  }
       //  else // updown collision
       //  {
       //    other_rigid->velocity.y = 0.f;
       //  }
       // update_transform(*other_rigid, *other_xform);
      // }
      // Technically we dont need false && true also cuz that would just be duplicated... i think
      // if both detect_collision is false then no point doing LOL
      // we wont do elastic collisions yet, cuz its not needed so i skip
    }
  );
  Engine::Perf::stop("Physics Update");
}

void phy2D::update_entity(Engine::ECS::Entity& entity) {
  Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
  Components::Transform2D* xform = entity.get_component<Components::Transform2D>();

  // Sync curr_pos with transform position at the start of the frame
  // This ensures curr_pos always reflects any external changes to transform.position
  rigid->curr_pos = xform->position;

  // Moved this down here because static objects can still have their position changed externally
  if (rigid->is_static) { return; }

  phy2D::update_kinematics(*rigid);
  phy2D::update_transform(*rigid, *xform);
  //std::cout << "velocity: " << rigid->velocity << " || position: " << rigid->curr_pos << std::endl;
}

void phy2D::update_kinematics(Components::RigidBody& rigid)
{
  // F = m x a => Force equals mass x acceleration
  // acceleration will contain a value if the entity is affected by gravity
  rigid.acceleration = rigid.force / rigid.mass + rigid.gravity_acc;
  // V = a x dt => Velocity equals acceleration x delta time
  // i increased friction for the x axis to test cuz we not sure if we want to do it like this
  Vec2D prev_velocity = Vec2D(rigid.velocity.x * 0.85f, rigid.velocity.y * rigid.friction);
  rigid.velocity = prev_velocity + rigid.acceleration * Time::get_fixed_dt();

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
  xform.position = rigid.curr_pos;
  xform.rotation += rigid.angular_velocity * Engine::Time::get_fixed_dt();
}

// Maybe we check for collision here and then do translate as the last step?

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
  rigid.velocity.x = -walking_velocity;
}

void phy2D::walk_right(Components::RigidBody& rigid)
{
  rigid.velocity.x = walking_velocity;
}

void phy2D::jump(Components::RigidBody& rigid)
{
  phy2D::add_force(rigid, jumping_force);
}