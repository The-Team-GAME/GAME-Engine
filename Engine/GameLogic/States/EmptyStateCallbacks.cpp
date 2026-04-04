//-----------------------------------------------------------------------------------
// file:    EmptyStateCallbacks.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Registers the default empty state machine blueprint with no callbacks.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"

REGISTER_STATE_MACHINE_BLUEPRINT(EmptyState, "Default",
  STATE("Default", NO_CALLBACK, NO_CALLBACK, NO_CALLBACK)
)


