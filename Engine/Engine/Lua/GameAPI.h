//-----------------------------------------------------------------------------------
// file:    GameAPI.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   GameAPI provides the interface between the C++ game engine and Lua scripts.
//          Handles registration of engine functionality to Lua state, component 
//          reflection, and entity manipulation from scripts. Supports runtime script
//          reloading and dynamic tag registration.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

// I used https://www.lua.org/docs.html to understand the different functions on how to load
// and run different scripts and functions in Lua from C++.

// I also used the Sol2 wrapper for template types https://github.com/ThePhD/sol2 to bind C++ functions.

#pragma once

#include "../MonkeyEngine.h"
#include <sol/sol.hpp>
#include <string>
#include <cstdint>

namespace Engine
{
  namespace GameAPI
  {

    using EntityId = uint32_t;

    struct EntityProxy
    {
      EntityId id;
    };


/**
 * @brief Registers all engine functionality and component types with the Lua state.
 * @param lua The Lua state to register functions and types with.
 * @details Exposes entity creation, destruction, component reflection, and resource management to Lua scripts.
 */
void register_api(sol::state& lua);

/**
 * @brief Creates a new entity in the ECS.
 * @return The EntityId of the newly created entity.
 */
EntityId create_entity();

/**
 * @brief Destroys an entity and all its components.
 * @param id The EntityId of the entity to destroy.
 */
void destroy_entity(EntityId id);

/**
 * @brief Adds a Transform2D component to an entity.
 * @param id The EntityId of the entity.
 * @param x X position.
 * @param y Y position.
 * @param rotation Rotation in radians.
 * @param sx Scale X.
 * @param sy Scale Y.
 */
void add_transform(EntityId id, f32 x, f32 y, f32 rotation, f32 sx, f32 sy);

/**
 * @brief Adds a Texture component to an entity using a file path.
 * @param id The EntityId of the entity.
 * @param path The file path to the texture.
 */
void add_texture(EntityId id, const std::string& path);

/**
 * @brief Adds a default rectangle texture to an entity.
 * @param id The EntityId of the entity.
 */
void add_texture_quad(EntityId id);

/**
 * @brief Adds a Color component to an entity.
 * @param id The EntityId of the entity.
 * @param r Red value (0-1).
 * @param g Green value (0-1).
 * @param b Blue value (0-1).
 * @param a Alpha value (0-1).
 */
void add_color(EntityId id, f32 r, f32 g, f32 b, f32 a);

/**
 * @brief Adds a RigidBody component to an entity.
 * @param id The EntityId of the entity.
 * @param is_kinematic True if the body is kinematic.
 * @param is_gravity True if gravity affects the body.
 */
void add_rigidbody(EntityId id, bool is_kinematic, bool is_gravity);

/**
 * @brief Adds a BoxCollider component to an entity.
 * @param id The EntityId of the entity.
 * @param width Width of the collider.
 * @param height Height of the collider.
 * @param is_static True if the collider is static.
 */
void add_box_collider(EntityId id, f32 width, f32 height, bool is_static);

/**
 * @brief Adds a tag component to an entity.
 * @param id The EntityId of the entity.
 * @param tag The tag string identifier.
 */
void add_tag(EntityId id, const std::string& tag);

/**
 * @brief Creates an entity from a Lua table definition.
 * @param t The Lua table containing the entity definition.
 * @return The EntityId of the created entity.
 */
EntityId create_from_table(const sol::table& t);

/**
 * @brief Retrieves all entities in the scene.
 * @return A vector of EntityProxy objects representing all entities.
 */
std::vector<EntityProxy> get_all_entities();

/**
 * @brief Template function to retrieve a component from an entity.
 * @tparam T The component type.
 * @param id The EntityId of the entity.
 * @return Pointer to the component, or nullptr if not found.
 */
template<typename T>
T* get_component(EntityId id);

/**
 * @brief Template function to register a component type with Lua.
 * @tparam T The component type.
 * @param lua The Lua state.
 * @param name The name to expose the component as in Lua.
 */
template<typename T>
void register_component_type(sol::state& lua, const std::string& name);

/**
 * @brief Template function to get a property from a component for Lua reflection.
 * @tparam T The component type.
 * @param component The component instance.
 * @param key The property name.
 * @param lua The Lua state view.
 * @return sol::object containing the property value or nil if not found.
 */
template<typename T>
sol::object reflect_get(T& component, sol::object key, sol::state_view lua);

/**
 * @brief Template function to set a property on a component from Lua.
 * @tparam T The component type.
 * @param component The component instance.
 * @param key The property name.
 * @param value The new value for the property.
 */
template<typename T>
void reflect_set(T& component, sol::object key, sol::object value);
  }
  
}