//-----------------------------------------------------------------------------------
// file:    Prefab.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the Prefab system for the ECS. Prefabs are reusable
//          entity templates that store component configurations. They can be
//          instantiated multiple times to create entities with the same setup.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Entity.h"
#include "Types.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include "../Reflection/ReflectionLibrary.h"

namespace Engine {
  namespace ECS {

    // Forward declarations
    class IComponentArray;

    /* ====================================== //
    // PrefabImpl - Reusable entity template  //
    // ====================================== */
    class PrefabImpl {
    private:
      ComponentSignature signature;
      std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>> component_storage;

    public:
      PrefabImpl() = default;

      /*=================================================================================
      Function Brief:
      Adds a component to this prefab template.

      Parameters:
      args - Arguments to construct the component.

      Where to call it:
      When building a prefab with specific components.
      ==================================================================================*/
      template<typename T, typename... Args>
      PrefabImpl& add_component(Args&&... args);

      /*=================================================================================
      Function Brief:
      Removes a component from this prefab template.

      Parameters:
      None (template parameter T specifies the component type).

      Where to call it:
      When modifying a prefab's component configuration.
      ==================================================================================*/
      template<typename T>
      PrefabImpl& remove_component();

      /*=================================================================================
      Function Brief:
      Retrieves a component from this prefab template.

      Parameters:
      None (template parameter T specifies the component type).

      Where to call it:
      When accessing or modifying a prefab's component data.
      ==================================================================================*/
      template<typename T>
      T* get_component();

      /*=================================================================================
      Function Brief:
      Retrieves a const component from this prefab template.

      Parameters:
      None (template parameter T specifies the component type).

      Where to call it:
      When reading a prefab's component data without modification.
      ==================================================================================*/
      template<typename T>
      const T* get_component() const;

      /*=================================================================================
      Function Brief:
      Checks if this prefab has a specific component.

      Parameters:
      None (template parameter T specifies the component type).

      Where to call it:
      When querying a prefab's component configuration.
      ==================================================================================*/
      template<typename T>
      bool has_component() const;

      /*=================================================================================
      Function Brief:
      Checks if this prefab has all specified components.

      Parameters:
      None (template parameters T... specify the component types).

      Where to call it:
      When checking multiple component requirements on a prefab.
      ==================================================================================*/
      template<typename... T>
      bool has_components() const;

      /*=================================================================================
      Function Brief:
      Gets the component signature of this prefab.

      Parameters:
      None.

      Where to call it:
      Internally when comparing or copying prefab configurations.
      ==================================================================================*/
      const ComponentSignature& get_signature() const;

      /*=================================================================================
      Function Brief:
      Gets mutable access to the component signature of this prefab.

      Parameters:
      None.

      Where to call it:
      Internally when modifying prefab configurations.
      ==================================================================================*/
      ComponentSignature& get_signature();

      /*=================================================================================
      Function Brief:
      Gets the component storage map of this prefab.

      Parameters:
      None.

      Where to call it:
      Internally when cloning components to/from entities.
      ==================================================================================*/
      const std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>>& get_component_storage() const;

      /*=================================================================================
      Function Brief:
      Gets mutable access to the component storage map of this prefab.

      Parameters:
      None.

      Where to call it:
      Internally when modifying prefab configurations.
      ==================================================================================*/
      std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>>& get_component_storage();

      /*=================================================================================
      Function Brief:
      Iterates over all components in this prefab.

      Parameters:
      callback - Function to call for each component.

      Where to call it:
      When processing all components in a prefab.
      ==================================================================================*/
      void for_each_component(std::function<void(const std::string&, IComponentArray*)> callback);
    };

    using Prefab = std::shared_ptr<PrefabImpl>;

    /* ====================================== //
    // Prefab functions                       //
    // ====================================== */

    /*=================================================================================
    Function Brief:
    Creates a new empty prefab.

    Parameters:
    None.

    Where to call it:
    When defining reusable entity templates.
    ==================================================================================*/
    Prefab create_prefab();

    /*=================================================================================
    Function Brief:
    Creates a new entity and copies all components from the prefab to it.

    Parameters:
    prefab - The prefab to instantiate.

    Where to call it:
    When spawning entities based on a prefab template.
    ==================================================================================*/
    Entity instantiate_prefab(const Prefab& prefab);

    /*=================================================================================
    Function Brief:
    Creates a prefab from an existing entity by copying its components.

    Parameters:
    entity - The entity to convert to a prefab.

    Where to call it:
    When creating a reusable template from an existing entity.
    ==================================================================================*/
    Prefab entity_to_prefab(const Entity& entity);

    /*=================================================================================
    Function Brief:
    Serializes a prefab to a GenericValue for saving/loading.

    Parameters:
    prefab - The prefab to serialize.

    Where to call it:
    When saving prefabs to files or transmitting over network.
    ==================================================================================*/
    ReflectionLibrary::GenericValue prefab_to_generic_value(const Prefab& prefab);

    /*=================================================================================
    Function Brief:
    Deserializes a GenericValue to create a prefab.

    Parameters:
    value - The GenericValue containing prefab data.

    Where to call it:
    When loading prefabs from files or receiving over network.
    ==================================================================================*/
    Prefab prefab_from_generic_value(const ReflectionLibrary::GenericValue& value);

  } // namespace ECS
} // namespace Engine