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
//          and the Lua binding side-registry (LuaBindingRegistry).
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Reflection/ReflectionLibrary.h"
#include "World.h"
#include "../LuaBindingRegistry.h"
#include "../LuaBindings.h"

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

3. struct CONCAT(AutoRegister_, __COUNTER__)
   - CONCAT(A, B) → AB (concatenates tokens)
   - __COUNTER__ → unique integer that increments with each use in a TU
   - Result: struct AutoRegister_42
   - Each macro use gets a UNIQUE struct name regardless of line number
   - __COUNTER__ is captured once via an _IMPL helper so every reference
     inside one macro expansion shares the same value

4. Constructor runs AUTOMATICALLY during static initialization (before main())
   - Calls register_component<CLASS_NAME>() to register with ECS
   - Pushes a Lua binding callback into LuaBindingRegistry

5. static AutoRegister_42 auto_register_instance_42;
   - Creates a static instance of the struct
   - Triggers the constructor to run
   - Happens BEFORE main() starts
===================================================================================
*/

// Helper function to register component for Lua binding (implemented in LuaComponentBindings.h)
namespace Engine {
  namespace LuaBindings {
    namespace Internal {
      void register_component_for_binding(std::function<void()> binding_func);

      template<typename T>
      void register_component_lua();
    }
  }
}

// Implementation helpers — receive the already-expanded counter value N so
// every use of it within the same macro expansion gets the same number.
#define REGISTER_COMPONENT_IMPL(CLASS_NAME, N) \
    namespace { \
        struct CONCAT(AutoRegister_, N) { \
            CONCAT(AutoRegister_, N)() { \
                Engine::ECS::register_component<CLASS_NAME>(); \
            } \
        }; \
        static CONCAT(AutoRegister_, N) CONCAT(auto_register_instance, N); \
    }

#define REGISTER_COMPONENT_LUA_IMPL(CLASS_NAME, N) \
    namespace { \
        struct CONCAT(AutoRegister_, N) { \
            CONCAT(AutoRegister_, N)() { \
                Engine::ECS::register_component<CLASS_NAME>(); \
                Engine::LuaBindings::Internal::register_component_lua<CLASS_NAME>(); \
            } \
        }; \
        static CONCAT(AutoRegister_, N) CONCAT(auto_register_instance, N); \
    }

#define REGISTER_COMPONENT_CUSTOM_IMPL(CLASS_NAME, N) \
    namespace { \
        struct CONCAT(AutoRegister_, N) { \
            CONCAT(AutoRegister_, N)() { \
                Engine::ECS::register_component<CLASS_NAME>(); \
            } \
        }; \
        static CONCAT(AutoRegister_, N) CONCAT(auto_register_instance, N); \
    }

#define REGISTER_COMPONENT_CUSTOM_LUA_IMPL(CLASS_NAME, N) \
    namespace { \
        struct CONCAT(AutoRegister_, N) { \
            CONCAT(AutoRegister_, N)() { \
                Engine::ECS::register_component<CLASS_NAME>(); \
                Engine::LuaBindings::Internal::register_component_lua<CLASS_NAME>(); \
            } \
        }; \
        static CONCAT(AutoRegister_, N) CONCAT(auto_register_instance, N); \
    }

// Standard component registration (ECS + Reflection only)
#define REGISTER_COMPONENT(CLASS_NAME, ...) \
    REFLECT_FIELDS(CLASS_NAME, __VA_ARGS__) \
    REGISTER_COMPONENT_IMPL(CLASS_NAME, __COUNTER__)

// Component registration with Lua binding (ECS + Reflection + Lua)
#define REGISTER_COMPONENT_LUA(CLASS_NAME, ...) \
    REFLECT_FIELDS(CLASS_NAME, __VA_ARGS__) \
    REGISTER_COMPONENT_LUA_IMPL(CLASS_NAME, __COUNTER__)

// Method binder for components — augments an already-registered Sol2 usertype
// with extra C++ methods. Use from GameLogic (or Engine) to expose methods
// that the auto-reflection binding doesn't capture (e.g. Health::damage).
//
// Usage:  LUA_BIND_METHODS(Components::Health, {
//           lua["Health"]["damage"] = &Components::Health::damage;
//           lua["Health"]["heal"]   = &Components::Health::heal;
//           lua["Health"]["is_dead"] = &Components::Health::is_dead;
//         })
#define LUA_BIND_METHODS(CLASS_NAME, BODY) \
    namespace { \
        struct CONCAT(AutoBindMethods_, __LINE__) { \
            CONCAT(AutoBindMethods_, __LINE__)() { \
                Engine::LuaBindingRegistry::instance().push_method_binder({ \
                    Engine::LuaBindings::short_name(#CLASS_NAME), \
                    [](sol::state& lua) { \
                        BODY \
                    } \
                }); \
            } \
        }; \
        static CONCAT(AutoBindMethods_, __LINE__) CONCAT(auto_bind_methods_instance, __LINE__); \
    }

// Custom reflector version (ECS + Custom Reflection + Lua binding)
#define REGISTER_COMPONENT_CUSTOM(CLASS_NAME, ...) \
    REFLECT_CUSTOM(CLASS_NAME, __VA_ARGS__) \
    REGISTER_COMPONENT_CUSTOM_IMPL(CLASS_NAME, __COUNTER__)

// Custom reflector with Lua binding (ECS + Custom Reflection + Lua)
#define REGISTER_COMPONENT_CUSTOM_LUA(CLASS_NAME, ...) \
    REFLECT_CUSTOM(CLASS_NAME, __VA_ARGS__) \
    REGISTER_COMPONENT_CUSTOM_LUA_IMPL(CLASS_NAME, __COUNTER__)
