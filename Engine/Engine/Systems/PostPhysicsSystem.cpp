//-----------------------------------------------------------------------------------
// file:    PhysicsSystem.cpp
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file defines a system that handles the post physics updates for all 
//          entities tht contain rigidbody and transform2d components.
//          Entities will have their rigidbody updated which in turn updates the
//          transform2d component as well with the forces applied during collision.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "PostPhysicsSystem.h"
using namespace Engine;
using namespace Systems;
using phy2D = Physics2DSystem;
using pphy2D = PostPhysics2DSystem;

pphy2D::PostPhysics2DSystem()
{
	require_components<Components::Transform2D, Components::RigidBody>();
}

void pphy2D::update_entity(ECS::Entity& entity)
{
	Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
	Components::Transform2D* xform = entity.get_component<Components::Transform2D>();
	phy2D::update_kinematics(*rigid);
	phy2D::update_position(*rigid, *xform);
	std::cout << "resultant velocity: " << rigid->velocity << std::endl;
}