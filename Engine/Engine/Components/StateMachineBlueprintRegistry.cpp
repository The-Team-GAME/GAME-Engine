//-----------------------------------------------------------------------------------
// file:    StateMachineBlueprintRegistry.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang
//
// email:   chew.s@digipen.edu
//
// brief:   Out-of-line singleton instance for StateMachineBlueprintRegistry.
//          Defined in a .cpp so the static local lives in exactly one module
//          (the Engine EXE) and is shared with any loaded DLLs via ENGINE_API.
//
// Copyright  2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "../Reflection/ReflectionLibrary.h"
#include "StateMachineBlueprintRegistry.h"

namespace Engine {

    StateMachineBlueprintRegistry& StateMachineBlueprintRegistry::instance() {
        // Leaked singleton — intentionally never destroyed so that
        // callbacks from hot-reloaded DLLs don't crash during static cleanup.
        static StateMachineBlueprintRegistry* registry = new StateMachineBlueprintRegistry();
        return *registry;
    }

} // namespace Engine
