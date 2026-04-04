//-----------------------------------------------------------------------------------
// file:    FunctionRegistry.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Out-of-line singleton instance for FunctionRegistry.
//          Defined in a .cpp so the static local lives in exactly one module
//          (the Engine EXE) and is shared with any loaded DLLs via ENGINE_API.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Reflection/ReflectionLibrary.h"
#include "FunctionRegistry.h"

namespace Engine {

    namespace { FunctionRegistry* g_func_registry = nullptr; }

    FunctionRegistry& FunctionRegistry::instance() {
        if (!g_func_registry) g_func_registry = new FunctionRegistry();
        return *g_func_registry;
    }

    void FunctionRegistry::destroy_instance() {
        delete g_func_registry;
        g_func_registry = nullptr;
    }

} // namespace Engine

