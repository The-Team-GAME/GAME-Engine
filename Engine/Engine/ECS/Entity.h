//-----------------------------------------------------------------------------------
// file:    Entity.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the Entity class which provides a clean handle-based
//          API for working with entities in the ECS. Entity acts as a lightweight
//          wrapper around an entity ID with convenient component access methods.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "Types.h"
#include "../Reflection/ReflectionLibrary.h"

namespace Engine {
  namespace ECS {

    /* ============================ //
    // Entity handle for clean API  //
    // ============================ */
    class Entity {
    private:
      EntityID id;

    public:
      /*=================================================================================
      Function Brief:
      Constructs an Entity handle with the specified ID.

      Parameters:
      id - The EntityID to wrap (defaults to INVALID_ENTITY).

      Where to call it:
      When creating entity handles from entity IDs.
      ==================================================================================*/
      Entity(EntityID id = INVALID_ENTITY);

      /*=================================================================================
      Function Brief:
      Returns the underlying EntityID of this entity handle.

      Parameters:
      None.

      Where to call it:
      When you need the raw entity ID for internal operations.
      ==================================================================================*/
      EntityID get_id() const;

      /*=================================================================================
      Function Brief:
      Checks if this entity handle is valid (not INVALID_ENTITY).

      Parameters:
      None.

      Where to call it:
      Before performing operations on an entity to ensure it's valid.
      ==================================================================================*/
      bool is_valid() const;

      /*=================================================================================
      Function Brief:
      Adds a component of type T to this entity, constructing it with the provided
      arguments.

      Parameters:
      args - Constructor arguments for the component.

      Where to call it:
      When attaching a new component to an entity.
      ==================================================================================*/
      template<typename T, typename... Args>
      Entity& add_component(Args&&... args);

      /*=================================================================================
      Function Brief:
      Removes a component of type T from this entity.

      Parameters:
      None (template parameter T specifies which component to remove).

      Where to call it:
      When detaching a component from an entity.
      ==================================================================================*/
      template<typename T>
      Entity& remove_component();

      /*=================================================================================
      Function Brief:
      Retrieves a pointer to the component of type T on this entity.

      Parameters:
      None (template parameter T specifies which component to get).

      Where to call it:
      When you need to access or modify a component's data.
      ==================================================================================*/
      template<typename T>
      T* get_component();

      /*=================================================================================
      Function Brief:
      Retrieves a const pointer to the component of type T on this entity.

      Parameters:
      None (template parameter T specifies which component to get).

      Where to call it:
      When you need to read a component's data without modifying it.
      ==================================================================================*/
      template<typename T>
      const T* get_component() const;

      /*=================================================================================
      Function Brief:
      Checks if this entity has a component of type T.

      Parameters:
      None (template parameter T specifies which component to check).

      Where to call it:
      Before accessing a component to verify it exists.
      ==================================================================================*/
      template<typename T>
      bool has_component() const;

      /*=================================================================================
      Function Brief:
      Checks if this entity has all the specified component types.

      Parameters:
      None (template parameters T... specify which components to check).

      Where to call it:
      When you need to verify an entity has multiple required components.
      ==================================================================================*/
      template<typename... T>
      bool has_components() const;

      /*=================================================================================
      Function Brief:
      Destroys this entity and removes all its components.

      Parameters:
      None.

      Where to call it:
      When you want to remove an entity from the world.
      ==================================================================================*/
      void destroy();

      /*=================================================================================
      Function Brief:
      Compares two entity handles for equality based on their IDs.

      Parameters:
      other - The entity to compare with.

      Where to call it:
      When checking if two entity handles refer to the same entity.
      ==================================================================================*/
      bool operator==(const Entity& other) const;

      /*=================================================================================
      Function Brief:
      Compares two entity handles for inequality based on their IDs.

      Parameters:
      other - The entity to compare with.

      Where to call it:
      When checking if two entity handles refer to different entities.
      ==================================================================================*/
      bool operator!=(const Entity& other) const;


    };

    Entity create_entity_from_generic_value(const ReflectionLibrary::GenericValue& value);
    ReflectionLibrary::GenericValue entity_to_generic_value(Entity entity);


  } // namespace ECS
} // namespace Engine

