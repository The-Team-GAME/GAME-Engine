//-----------------------------------------------------------------------------------
// file:    EventManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the EventTypeManager static member initialization.
//          Initializes the counter used for automatic event type ID generation.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "EventManager.h"

namespace ECS
{
  EventTypeID EventTypeManager::next_type_id = 0;
}