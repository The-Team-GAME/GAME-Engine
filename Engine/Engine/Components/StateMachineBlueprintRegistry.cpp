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

    namespace { StateMachineBlueprintRegistry* g_bp_registry = nullptr; }

    StateMachineBlueprintRegistry& StateMachineBlueprintRegistry::instance() {
        if (!g_bp_registry) g_bp_registry = new StateMachineBlueprintRegistry();
        return *g_bp_registry;
    }

    void StateMachineBlueprintRegistry::destroy_instance() {
        delete g_bp_registry;
        g_bp_registry = nullptr;
    }

} // namespace Engine
