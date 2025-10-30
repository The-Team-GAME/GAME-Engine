//-----------------------------------------------------------------------------------
// file:    ECS.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This is the main header file for the ECS (Entity Component System).
//          Includes all ECS headers and their inline implementations, providing
//          a single include point for using the entire ECS framework.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "SparseSet.h"
#include "ComponentManager.h"
#include "ComponentArray.h"
#include "Entity.h"
#include "System.h"
#include "World.h"
#include "WorldData.h"
#include "Prefab.h"
#include "Macros.h"
#include "Entity.inl"
#include "World.inl"
#include "System.inl"
#include "ComponentManager.inl"
#include "ComponentArray.inl"