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
#include "CollisionSystem.h"

using namespace Engine;
using namespace Systems;
using coll2D = Collision2DSystem;

coll2D::Collision2DSystem() {
  require_components<Components::RigidBody, Components::BoxCollider>();
}
