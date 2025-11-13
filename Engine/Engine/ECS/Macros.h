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
//          that automatically register components with the ECS system and
//          set up reflection information for the editor interface.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Reflection/ReflectionLibrary.h"
#include "World.h"


// Macro to reflect and register a component with the ECS
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

// Macro for components with custom reflectors
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