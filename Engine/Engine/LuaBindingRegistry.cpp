//-----------------------------------------------------------------------------------
// file:    LuaBindingRegistry.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of the LuaBindingRegistry singleton.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Precompiled.h"
#include "LuaBindingRegistry.h"

namespace Engine {

  LuaBindingRegistry& LuaBindingRegistry::instance() {
    static LuaBindingRegistry* inst = new LuaBindingRegistry();
    return *inst;
  }

  void LuaBindingRegistry::destroy_instance() {
    // Intentional leak pattern matching FunctionRegistry / StateMachineBlueprintRegistry
    // The singleton is heap-allocated to survive DLL unload order issues.
    // destroy_instance() is called at final shutdown.
  }

} // namespace Engine


