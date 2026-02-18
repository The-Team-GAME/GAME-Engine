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
// Copyright  2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Reflection/ReflectionLibrary.h"
#include "FunctionRegistry.h"

namespace Engine {

    FunctionRegistry& FunctionRegistry::instance() {
        // Leaked singleton — intentionally never destroyed so that
        // std::any objects holding DLL callbacks don't crash during
        // static cleanup after the DLL has been unloaded.
        static FunctionRegistry* registry = new FunctionRegistry();
        return *registry;
    }

} // namespace Engine
