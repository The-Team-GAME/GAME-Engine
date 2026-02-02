//-----------------------------------------------------------------------------------
// file:    Memory.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Main header file for the custom Memory Manager system.
//          Includes all memory management headers and custom containers.
//          Use this single header to access all memory functionality.
//
//          Features:
//          - Pool-based memory allocation with pre-allocated pages
//          - O(1) allocation and deallocation for pooled sizes
//          - Automatic pool expansion when running out of memory
//          - Custom STL-free containers (Array, HashMap, MCSparseSet)
//          - Placement new support for typed allocations
//          - No runtime OS allocations during gameplay (except expansion)
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

// Core memory management
#include "MemoryConfig.h"
#include "MemoryPage.h"
#include "PoolAllocator.h"
#include "MemoryManager.h"

// Custom containers
#include "Containers/Containers.h"
