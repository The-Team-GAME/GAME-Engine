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

// Custom Memory Manager - must be included before other ECS headers
#include "../Memory/MemoryManager.h"
#include "ManagedSparseSet.h"
#include "ManagedContainers.h"
#include "ManagedComponentArray.h"

#include "Types.h"
#include "ComponentManager.h"
#include "Entity.h"
#include "System.h"
#include "LayerRegistry.h"
#include "World.h"
#include "WorldData.h"
#include "Prefab.h"
#include "Reflection.h"
#include "Macros.h"
#include "Entity.inl"
#include "World.inl"
#include "System.inl"
#include "ComponentManager.inl"
#include "Prefab.inl"