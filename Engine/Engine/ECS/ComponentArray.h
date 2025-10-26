//-----------------------------------------------------------------------------------
// file:    ComponentArray.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the component storage interface and type-safe component
//          array template class for the ECS (Entity Component System). ComponentArray
//          stores components of a specific type and provides fast lookups by entity ID.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include <unordered_map>
#include "../Reflection/ReflectionLibrary.h"

namespace Engine {
  namespace ECS {

    /* ============================ //
    // Component storage interface  //
    // ============================ */
    class IComponentArray {
    public:
      virtual ~IComponentArray() = default;
      virtual void remove_entity(EntityID entity) = 0;
      virtual void clone_component(EntityID source, EntityID destination) = 0;
      virtual bool is_reflectable() const = 0;
      virtual std::string get_type_name() const = 0;
      virtual ReflectionLibrary::GenericValue to_generic_value(EntityID entity) const = 0;
      virtual bool add_component_from_generic_value(EntityID entity, const ReflectionLibrary::GenericValue& value) = 0;
      virtual bool set_component_from_generic_value(EntityID entity, const ReflectionLibrary::GenericValue& value) = 0;
      virtual void add_default_component(EntityID entity) = 0;
    };

    /* ============================= //
    // Type-safe component storage   //
    // ============================= */
    template<typename T>
    class ComponentArray : public IComponentArray {
    private:
      std::unordered_map<EntityID, T> components;

    public:
      /*=================================================================================
      Function Brief:
      Adds a component of type T to the specified entity. If the entity already has
      a component of this type, it will be replaced.

      Parameters:
      entity - The EntityID to add the component to.
      component - The component instance to add (moved into storage).

      Where to call it:
      When you need to attach a new component to an entity in the ECS.
      ==================================================================================*/
      void add_component(EntityID entity, const T& component);

      /*=================================================================================
      Function Brief:
      Clones a component from one entity to another. Creates a copy of the source
      entity's component and adds it to the destination entity.

      Parameters:
      source - The EntityID to copy the component from.
      destination - The EntityID to copy the component to.

      Where to call it:
      When duplicating entities or copying component data between entities.
      ==================================================================================*/
      void clone_component(EntityID source, EntityID destination) override;

      /*=================================================================================
      Function Brief:
      Retrieves a pointer to the component of type T for the specified entity.

      Parameters:
      entity - The EntityID to get the component from.

      Where to call it:
      When you need to access or modify a component's data.
      ==================================================================================*/
      T* get_component(EntityID entity);

      /*=================================================================================
      Function Brief:
      Checks if the specified entity has a component of type T.

      Parameters:
      entity - The EntityID to check.

      Where to call it:
      When you need to verify if an entity has a specific component before accessing it.
      ==================================================================================*/
      bool has_component(EntityID entity) const;

      /*=================================================================================
      Function Brief:
      Removes a component of type T from the specified entity.

      Parameters:
      entity - The EntityID to remove the component from.

      Where to call it:
      When you need to detach a component from an entity.
      ==================================================================================*/
      void remove_entity(EntityID entity) override;


      /*=================================================================================
      Function Brief:
      Checks if the component type T is reflectable using the ReflectionLibrary.
      Parameters:
      None.
      Where to call it:
      When determining if the component can be serialized or inspected via reflection.
      ==================================================================================*/
      bool is_reflectable() const override;

      /*=================================================================================
      Function Brief:
      Returns the type name of the component as a string.

      Parameters:
      None.

      Where to call it:
      When serializing components or debugging component types.
      ==================================================================================*/
      std::string get_type_name() const override;

      /*=================================================================================
      Function Brief:
      Converts a component to a GenericValue for serialization or inspection.

      Parameters:
      entity - The EntityID whose component to convert.

      Where to call it:
      When serializing component data or inspecting it at runtime.
      ==================================================================================*/
      ReflectionLibrary::GenericValue to_generic_value(EntityID entity) const override;

      /*=================================================================================
      Function Brief:
      Adds a new component to an entity from a GenericValue.

      Parameters:
      entity - The EntityID to add the component to.
      value - The GenericValue containing the component data.

      Where to call it:
      When deserializing entities from JSON or creating components from runtime data.
      ==================================================================================*/
      bool add_component_from_generic_value(EntityID entity, const ReflectionLibrary::GenericValue& value) override;

      /*=================================================================================
      Function Brief:
      Updates an existing component's data from a GenericValue.

      Parameters:
      entity - The EntityID whose component to update.
      value - The GenericValue containing the new component data.

      Where to call it:
      When modifying component data from UI editors or runtime updates.
      ==================================================================================*/
      bool set_component_from_generic_value(EntityID entity, const ReflectionLibrary::GenericValue& value) override;

      /*=================================================================================
      Function Brief:
      Adds a default-constructed component of type T to the specified entity.
      
      Parameters:
      entity - The EntityID to add the default component to.
      
      Where to call it:
      When you need to attach a default-initialized component to an entity.
      ==================================================================================*/ 
      void add_default_component(EntityID entity);

      /*=================================================================================
      Function Brief:
      Returns an iterator to the beginning of the component storage.

      Parameters:
      None.

      Where to call it:
      When iterating over all components in this array.
      ==================================================================================*/
      auto begin();

      /*=================================================================================
      Function Brief:
      Returns an iterator to the end of the component storage.

      Parameters:
      None.

      Where to call it:
      When iterating over all components in this array.
      ==================================================================================*/
      auto end();

      /*=================================================================================
      Function Brief:
      Returns a const iterator to the beginning of the component storage.

      Parameters:
      None.

      Where to call it:
      When iterating over all components in this array without modifying them.
      ==================================================================================*/
      auto begin() const;

      /*=================================================================================
      Function Brief:
      Returns a const iterator to the end of the component storage.

      Parameters:
      None.

      Where to call it:
      When iterating over all components in this array without modifying them.
      ==================================================================================*/
      auto end() const;
    };

  } // namespace ECS
} // namespace Engine