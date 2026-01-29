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
#include <functional>
#include <string>
#include "Types.h"
#include "../Reflection/ReflectionLibrary.h"

namespace Engine {
  namespace ECS {

    // Forward declaration
    class IManagedComponentArray;

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
        requires std::is_constructible_v<T, Args...>
      Entity& add_component(Args&&... args);

      /*=================================================================================
      Function Brief:
      Adds a component to this entity by component type name at runtime.
      Creates a default-constructed instance of the component.

      Parameters:
      component_name - The registered name of the component type.

      Returns:
      True if the component was successfully added, false if the component type
      was not found or could not be added.

      Where to call it:
      When attaching a component to an entity using a string name (e.g., from UI).
      ==================================================================================*/
      bool add_component_by_name(const std::string& component_name);

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

      /*=================================================================================
      Function Brief:
      Iterates through all components attached to this entity, calling the provided
      callback for each component.

      Parameters:
      callback - Function called for each component with (component_type_name, component_array)

      Where to call it:
      When you need to iterate through all components an entity has.
      ==================================================================================*/
      void for_each_component(std::function<void(const std::string&, IManagedComponentArray*)> callback);

      /*=================================================================================
      Function Brief:
      Adds this entity to one or more layers.

      Parameters:
      None (template parameters Layers... specify which layers to add to).

      Where to call it:
      When you want to add an entity to additional layers.
      ==================================================================================*/
      template<LayerID... Layers>
      Entity& add_to_layers();

      /*=================================================================================
      Function Brief:
      Removes this entity from one or more layers.

      Parameters:
      None (template parameters Layers... specify which layers to remove from).

      Where to call it:
      When you want to remove an entity from specific layers.
      ==================================================================================*/
      template<LayerID... Layers>
      Entity& remove_from_layers();

      /*=================================================================================
      Function Brief:
      Sets the exact layers this entity belongs to (replaces existing layers).

      Parameters:
      None (template parameters Layers... specify which layers the entity should be on).

      Where to call it:
      When you want to explicitly define which layers an entity is on.
      ==================================================================================*/
      template<LayerID... Layers>
      Entity& set_layers();

      /*=================================================================================
      Function Brief:
      Checks if this entity is on a specific layer.

      Parameters:
      layer - The layer ID to check for.

      Where to call it:
      When you need to know if an entity is on a specific layer.
      ==================================================================================*/
      bool is_on_layer(LayerID layer) const;

      /*=================================================================================
      Function Brief:
      Checks if this entity is on ANY of the specified layers.

      Parameters:
      None (template parameters Layers... specify which layers to check).

      Where to call it:
      When you need to know if an entity is on at least one of several layers.
      ==================================================================================*/
      template<LayerID... Layers>
      bool is_on_any_layer() const;

      /*=================================================================================
      Function Brief:
      Checks if this entity is on ALL of the specified layers.

      Parameters:
      None (template parameters Layers... specify which layers to check).

      Where to call it:
      When you need to know if an entity is on all specified layers.
      ==================================================================================*/
      template<LayerID... Layers>
      bool is_on_all_layers() const;

      /*=================================================================================
      Function Brief:
      Gets all layers this entity belongs to.

      Parameters:
      None.

      Where to call it:
      When you need the full layer signature of an entity.
      ==================================================================================*/
      LayerSignature get_layers() const;

      /*=================================================================================
      Function Brief:
      Removes this entity from all layers.

      Parameters:
      None.

      Where to call it:
      When you want to remove an entity from all layers.
      ==================================================================================*/
      Entity& clear_layers();

      /*=================================================================================
      Function Brief:
      Adds this entity to a layer by name.

      Parameters:
      layer_name - The name of the layer to add to.

      Where to call it:
      When you want to add an entity to a layer using its name.
      ==================================================================================*/
      Entity& add_to_layer(const std::string& layer_name);

      /*=================================================================================
      Function Brief:
      Removes this entity from a layer by name.

      Parameters:
      layer_name - The name of the layer to remove from.

      Where to call it:
      When you want to remove an entity from a layer using its name.
      ==================================================================================*/
      Entity& remove_from_layer(const std::string& layer_name);

      /*=================================================================================
      Function Brief:
      Checks if this entity is on a layer by name.

      Parameters:
      layer_name - The name of the layer to check for.

      Where to call it:
      When you need to know if an entity is on a specific layer by name.
      ==================================================================================*/
      bool is_on_layer(const std::string& layer_name) const;

    };

    ReflectionLibrary::GenericValue entity_to_generic_value(Entity entity);
    Entity create_entity_from_generic_value(const ReflectionLibrary::GenericValue& value);


  } // namespace ECS
} // namespace Engine

