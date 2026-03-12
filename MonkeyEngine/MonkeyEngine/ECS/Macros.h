//-----------------------------------------------------------------------------------
// file:    Macros.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Macro definitions for ECS component registration and reflection.
//          Provides REGISTER_COMPONENT and REGISTER_COMPONENT_CUSTOM macros
//          that automatically register components with the ECS system, reflection,
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Reflection/ReflectionLibrary.h"
#include "World.h"

/*
===================================================================================
HOW THESE MACROS WORK:

1. REFLECT_FIELDS(CLASS_NAME, field1, field2, ...)
   - Creates a ReflectionInfo<CLASS_NAME> template specialization
   - Stores field names and pointers-to-members in a tuple
   - Example: ReflectionInfo<RigidBody>::fields = {{"mass", &RigidBody::mass}, ...}

2. namespace { ... }
   - Anonymous namespace = private to this translation unit
   - Prevents naming conflicts across files

3. struct CONCAT(AutoRegister_, __LINE__)
   - CONCAT(A, B) → AB (concatenates tokens)
   - __LINE__ → current line number (e.g., 42)
   - Result: struct AutoRegister_42
   - Each macro use gets a UNIQUE struct name

4. Constructor runs AUTOMATICALLY during static initialization (before main())
   - Calls register_component<CLASS_NAME>() to register with ECS
5. static AutoRegister_42 auto_register_instance_42;
   - Creates a static instance of the struct
   - Triggers the constructor to run
   - Happens BEFORE main() starts
===================================================================================
*/

// Standard component registration (ECS + Reflection only)
#define REGISTER_COMPONENT(CLASS_NAME, ...) \
    REFLECT_FIELDS(CLASS_NAME, __VA_ARGS__) \
    namespace { \
        struct CONCAT(AutoRegister_, __LINE__) { \
            CONCAT(AutoRegister_, __LINE__)() { \
                Engine::ECS::register_component<CLASS_NAME>(); \
            } \
        }; \
        static CONCAT(AutoRegister_, __LINE__) CONCAT(auto_register_instance, __LINE__); \
    }

// Custom reflector version (ECS + Custom Reflection only)
#define REGISTER_COMPONENT_CUSTOM(CLASS_NAME, ...) \
    REFLECT_CUSTOM(CLASS_NAME, __VA_ARGS__) \
    namespace { \
        struct CONCAT(AutoRegister_, __LINE__) { \
            CONCAT(AutoRegister_, __LINE__)() { \
                Engine::ECS::register_component<CLASS_NAME>(); \
            } \
        }; \
        static CONCAT(AutoRegister_, __LINE__) CONCAT(auto_register_instance, __LINE__); \
    }

