//-----------------------------------------------------------------------------------
// file:    HierarchyComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for entity hierarchy system (not yet implemented).
//          Defines Parent, Children, Root, and Sibling components for future
//          implementation of parent-child relationships between entities.
//          Will enable scene graph functionality and hierarchical transformations.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <vector>
#include "../ECS/ECS.h"

// NOT USED YET, WILL BE IMPLEMENTED IN THE FUTURE
namespace Components
{
  struct Parent {
    Engine::ECS::Entity parent_id;
  };
  struct Children { std::vector<Engine::ECS::Entity> child_ids; };
  struct Root {}; // Mark root entities
  struct Sibling { Engine::ECS::Entity next_sibling; }; // Maybe don't need this
}