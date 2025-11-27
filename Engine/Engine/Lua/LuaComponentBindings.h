//-----------------------------------------------------------------------------------
// file:    LuaComponentBindings.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   Automatic Lua binding system for ECS components using the reflection
//          library. Uses ReflectionLibrary to automatically expose component fields
//          to Lua without manual wrapper code.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#pragma once

#include "LuaManager.h"
#include "../ECS/Entity.h"
#include "../Reflection/ReflectionLibrary.h"
#include "../Math/Vector2D.h"
#include "../Components/PhysicsComponents.h"
#include "../Components/SpatialComponents.h"
#include "../Components/GraphicsComponents.h"
#include "../Components/StateMachineComponents.h"
#include "../../GameLogic/Components/GameplayComponent.h"

namespace Engine
{
  namespace LuaBindings
  {
    // Storage for deferred component binding
    namespace Internal {
      inline std::vector<std::function<void()>>& get_pending_bindings() {
        static std::vector<std::function<void()>> bindings;
        return bindings;
      }

      // Registration function called by REGISTER_COMPONENT_LUA macro
      inline void register_component_for_binding(std::function<void()> binding_func) {
        get_pending_bindings().push_back(binding_func);
      }

      // Helper template function that macros can call directly
      template<typename T>
      void register_component_lua() {
        register_component_for_binding([]() { bind_component<T>(); });
      }

      // Helper to add custom methods to a component usertype (override for specific types)
      template<typename T>
      void add_component_methods(sol::usertype<T>& component_type) {
        // Default: no custom methods
      }

      // Specialization for StateMachine
      template<>
      inline void add_component_methods<Components::StateMachine>(sol::usertype<Components::StateMachine>& component_type) {
        component_type["change_state"] = &Components::StateMachine::change_state;
        component_type["set_initial_state"] = &Components::StateMachine::set_initial_state;
      }

      // Template function to bind a single component (called via function pointer)
      template<typename T>
      void bind_component() {
        sol::state& lua = LuaManager::get_state();
        
        if (!lua.lua_state()) {
          std::cerr << "[LuaBindings] Error: Lua not initialized when binding component\n";
          return;
        }
        
        // Get component type name from reflection
        constexpr auto type_name = ReflectionLibrary::class_name<T>;
        
        // Create Lua usertype for this component
        auto component_type = lua.new_usertype<T>(type_name.data(),
          sol::no_constructor  // Prevent Lua from creating components directly
        );
        
        // Use reflection to iterate over all fields and bind them
        auto field_tuple = ReflectionLibrary::ReflectionInfo<T>::fields;
        std::apply([&](auto... field_pairs) {
          // Bind each field using pointer-to-member
          ((component_type[field_pairs.first] = field_pairs.second), ...);
        }, field_tuple);
        
        // Add any custom methods for this type
        add_component_methods<T>(component_type);
        
        std::cout << "[LuaBindings] Auto-bound component: " << type_name << std::endl;
      }
    }

    /**
     * @brief Binds Math::Vec2D type to Lua.
     */
    inline void bind_vec2d_type() {
      sol::state& lua = LuaManager::get_state();
      
      auto vec2_type = lua.new_usertype<Math::Vec2D>("Vec2D",
        sol::constructors<Math::Vec2D(), Math::Vec2D(float, float)>()
      );
      vec2_type["x"] = &Math::Vec2D::x;
      vec2_type["y"] = &Math::Vec2D::y;
      
      std::cout << "[LuaBindings] Vec2D type bound to Lua\n";
    }

    /**
     * @brief Manually bind Animation component (can't use auto-binding due to custom Reflector).
     */
    inline void bind_animation_component() {
      sol::state& lua = LuaManager::get_state();
      
      auto anim_type = lua.new_usertype<Components::Animation>("Animation",
        sol::no_constructor
      );
      
      // Bind the has_ended method that Lua scripts need
      anim_type["has_ended"] = &Components::Animation::has_ended;
      
      std::cout << "[LuaBindings] Animation component bound to Lua (manual)\n";
    }



    /**
     * @brief Registers Entity class with generic component access.
     */
    inline void bind_entity_type() {
      sol::state& lua = LuaManager::get_state();

      auto entity_type = lua.new_usertype<ECS::Entity>("Entity",
        sol::no_constructor
      );

      // Generic get_component using string name
      entity_type["get_component"] = [](ECS::Entity& e, const std::string& component_name) -> sol::object {
        sol::state& lua = LuaManager::get_state();
        
        // Map component names to actual types
        if (component_name == "RigidBody") {
          auto* comp = e.get_component<Components::RigidBody>();
          if (comp) return sol::make_object(lua, comp);
        }
        else if (component_name == "Transform2D") {
          auto* comp = e.get_component<Components::Transform2D>();
          if (comp) return sol::make_object(lua, comp);
        }
        else if (component_name == "Flip") {
          auto* comp = e.get_component<Components::Flip>();
          if (comp) return sol::make_object(lua, comp);
        }
        else if (component_name == "LazyMonkey") {
          auto* comp = e.get_component<Components::LazyMonkey>();
          if (comp) return sol::make_object(lua, comp);
        }
        else if (component_name == "Animation") {
          auto* comp = e.get_component<Components::Animation>();
          if (comp) return sol::make_object(lua, comp);
        }
        else if (component_name == "StateMachine") {
          auto* comp = e.get_component<Components::StateMachine>();
          if (comp) return sol::make_object(lua, comp);
        }
        else if (component_name == "Player") {
          auto* comp = e.get_component<Components::Player>();
          if (comp) return sol::make_object(lua, comp);
        }
        else if (component_name == "PendingKnockback") {
          auto* comp = e.get_component<Components::PendingKnockback>();
          if (comp) return sol::make_object(lua, comp);
        }
        
        return sol::nil;
      };

      // Has component check
      entity_type["has_component"] = [](ECS::Entity& e, const std::string& component_name) -> bool {
        if (component_name == "Parent") {
          return e.has_component<Components::Parent>();
        }
        else if (component_name == "PendingKnockback") {
          return e.has_component<Components::PendingKnockback>();
        }
        else if (component_name == "RigidBody") {
          return e.has_component<Components::RigidBody>();
        }
        else if (component_name == "StateMachine") {
          return e.has_component<Components::StateMachine>();
        }
        return false;
      };
      
      // Remove component
      entity_type["remove_component"] = [](ECS::Entity& e, const std::string& component_name) {
        if (component_name == "Parent") {
          e.remove_component<Components::Parent>();
        }
        else if (component_name == "PendingKnockback") {
          e.remove_component<Components::PendingKnockback>();
        }
      };

      entity_type["is_valid"] = &ECS::Entity::is_valid;
      
      std::cout << "[LuaBindings] Entity type bound to Lua\n";
    }

    /**
     * @brief Binds ECS utility functions.
     */
    inline void bind_ecs_functions() {
      sol::state& lua = LuaManager::get_state();
      
      auto ecs_table = lua["ECS"].get_or_create<sol::table>();
      
      // Get first entity with component
      ecs_table["get_first_entity_with"] = [](const std::string& component_name) -> ECS::Entity {
        if (component_name == "Player") {
          return ECS::get_first_entity_with<Components::Player>();
        }
        else if (component_name == "LazyMonkey") {
          return ECS::get_first_entity_with<Components::LazyMonkey>();
        }
        return ECS::Entity{};
      };
      
      // Destroy entity
      ecs_table["destroy_entity"] = [](ECS::Entity& entity) {
        ECS::destroy_entity(entity);
      };
      
      std::cout << "[LuaBindings] ECS functions bound to Lua\n";
    }

    /**
     * @brief Binds Time utility functions.
     */
    inline void bind_time_functions() {
      sol::state& lua = LuaManager::get_state();
      
      auto time_table = lua["Time"].get_or_create<sol::table>();
      time_table["get_time"] = &Engine::Time::get_time;
      
      std::cout << "[LuaBindings] Time functions bound to Lua\n";
    }

    /**
     * @brief Registers all Lua bindings for the game using reflection.
     * Call this after LuaManager::init() but before loading scripts.
     * Note: Components are auto-bound via REGISTER_COMPONENT_LUA macro.
     */
    inline void register_all_bindings() {
      // Bind basic types first
      bind_vec2d_type();
      bind_entity_type();
      bind_ecs_functions();
      bind_time_functions();
      
      // Bind components that need manual binding (can't use auto-binding)
      bind_animation_component();
      
      // Process all deferred component bindings registered via REGISTER_COMPONENT_LUA
      auto& pending = Internal::get_pending_bindings();
      std::cout << "[LuaBindings] Processing " << pending.size() << " deferred component bindings...\n";
      for (auto& binding : pending) {
        binding();
      }
      pending.clear();
      
      std::cout << "[LuaBindings] All bindings registered (components auto-bound via REGISTER_COMPONENT_LUA)\n";
    }
  }
}