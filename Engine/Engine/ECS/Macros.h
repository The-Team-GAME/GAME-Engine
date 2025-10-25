#pragma once

#include "../Reflection/ReflectionLibrary.h"
#include "World.h"


// Macro to reflect and register a component with the ECS
#define REFLECT_AND_REGISTER_COMPONENT(CLASS_NAME, ...) \
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
#define REFLECT_AND_REGISTER_COMPONENT_CUSTOM(CLASS_NAME, ...) \
    REFLECT_CUSTOM(CLASS_NAME, __VA_ARGS__) \
    namespace { \
        struct CONCAT(AutoRegister_, __LINE__) { \
            CONCAT(AutoRegister_, __LINE__)() { \
                Engine::ECS::register_component<CLASS_NAME>(); \
            } \
        }; \
        static CONCAT(AutoRegister_, __LINE__) CONCAT(auto_register_instance, __LINE__); \
    }