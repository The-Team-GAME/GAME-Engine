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
#include "ComponentArray.h"
#include "../EngineExport.h"
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
      LayerSignature layers;
      std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>> component_storage;
      std::string prefab_id; // Unique UUID identifier for this prefab
      std::string prefab_filepath; // Filepath where this prefab is stored
      std::vector<std::string> load_errors; // Errors encountered during loading

    public:
      PrefabImpl() = default;
      PrefabImpl(const PrefabImpl&) = delete;
      PrefabImpl& operator=(const PrefabImpl&) = delete;
      ENGINE_API PrefabImpl(PrefabImpl&&) noexcept;
      ENGINE_API PrefabImpl& operator=(PrefabImpl&&) noexcept;
      ENGINE_API ~PrefabImpl();

      /*=================================================================================
      Function Brief:
      Checks if this prefab has any loading errors.

      Parameters:
      None.

      Where to call it:
      When validating prefab before instantiation.
      ==================================================================================*/
      bool has_load_errors() const { return !load_errors.empty(); }

      /*=================================================================================
      Function Brief:
      Gets the list of loading errors for this prefab.

      Parameters:
      None.

      Where to call it:
      When displaying error information to the user.
      ==================================================================================*/
      const std::vector<std::string>& get_load_errors() const { return load_errors; }

      /*=================================================================================
      Function Brief:
      Adds a loading error to this prefab.

      Parameters:
      error - The error message to add.

      Where to call it:
      During prefab deserialization when an error is encountered.
      ==================================================================================*/
      void add_load_error(const std::string& error) { load_errors.push_back(error); }

      /*=================================================================================
      Function Brief:
      Clears all loading errors from this prefab.

      Parameters:
      None.

      Where to call it:
      When reloading or resetting a prefab.
      ==================================================================================*/
      void clear_load_errors() { load_errors.clear(); }

      /*=================================================================================
      Function Brief:
      Adds a component to this prefab template.

      Parameters:
      args - Arguments to construct the component.

      Where to call it:
      When building a prefab with specific components.
      ==================================================================================*/
      template<typename T, typename... Args>
        requires std::is_constructible_v<T, Args...>
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
      ENGINE_API const ComponentSignature& get_signature() const;

      /*=================================================================================
      Function Brief:
      Gets mutable access to the component signature of this prefab.

      Parameters:
      None.

      Where to call it:
      Internally when modifying prefab configurations.
      ==================================================================================*/
      ENGINE_API ComponentSignature& get_signature();

      /*=================================================================================
      Function Brief:
      Gets the component storage map of this prefab.

      Parameters:
      None.

      Where to call it:
      Internally when cloning components to/from entities.
      ==================================================================================*/
      ENGINE_API const std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>>& get_component_storage() const;

      /*=================================================================================
      Function Brief:
      Gets mutable access to the component storage map of this prefab.

      Parameters:
      None.

      Where to call it:
      Internally when modifying prefab configurations.
      ==================================================================================*/
      ENGINE_API std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>>& get_component_storage();

      /*=================================================================================
      Function Brief:
      Gets the layer signature of this prefab.

      Parameters:
      None.

      Where to call it:
      When accessing prefab layer configuration.
      ==================================================================================*/
      ENGINE_API const LayerSignature& get_layers() const;

      /*=================================================================================
      Function Brief:
      Gets mutable access to the layer signature of this prefab.

      Parameters:
      None.

      Where to call it:
      When modifying prefab layer configuration.
      ==================================================================================*/
      ENGINE_API LayerSignature& get_layers();

      /*=================================================================================
      Function Brief:
      Iterates over all components in this prefab.

      Parameters:
      callback - Function to call for each component.

      Where to call it:
      When processing all components in a prefab.
      ==================================================================================*/
      ENGINE_API void for_each_component(std::function<void(const std::string&, IComponentArray*)> callback);

      /*=================================================================================
      Function Brief:
      Gets the prefab ID (UUID).

      Parameters:
      None.

      Where to call it:
      When tracking which prefab an instance was created from.
      ==================================================================================*/
      ENGINE_API const std::string& get_prefab_id() const;

      /*=================================================================================
      Function Brief:
      Sets the prefab ID (UUID).

      Parameters:
      id - The UUID to set.

      Where to call it:
      When creating a new prefab or loading from file.
      ==================================================================================*/
      ENGINE_API void set_prefab_id(const std::string& id);

      /*=================================================================================
      Function Brief:
      Gets the prefab filepath.

      Parameters:
      None.

      Where to call it:
      When tracking the location where the prefab is stored.
      ==================================================================================*/
      ENGINE_API const std::string& get_prefab_filepath() const;

      /*=================================================================================
      Function Brief:
      Sets the prefab filepath.

      Parameters:
      filepath - The filepath to set.

      Where to call it:
      When loading a prefab from file or saving to a new location.
      ==================================================================================*/
      ENGINE_API void set_prefab_filepath(const std::string& filepath);
    };

    /* ============================ //
    // Prefab handle for clean API  //
    // ============================ */
    class Prefab {
    private:
      std::shared_ptr<PrefabImpl> impl;

    public:
      /*=================================================================================
      Function Brief:
      Constructs an empty Prefab handle.

      Parameters:
      None.

      Where to call it:
      When creating a null prefab handle.
      ==================================================================================*/
      Prefab() = default;

      ENGINE_API ~Prefab();

      /*=================================================================================
      Function Brief:
      Constructs a Prefab handle wrapping a PrefabImpl.

      Parameters:
      impl - Shared pointer to the PrefabImpl.

      Where to call it:
      Internally when creating prefabs.
      ==================================================================================*/
      ENGINE_API Prefab(std::shared_ptr<PrefabImpl> impl);

      /*=================================================================================
      Function Brief:
      Adds a component to this prefab with builder pattern.

      Parameters:
      args - Arguments to construct the component.

      Where to call it:
      When building a prefab with specific components.
      ==================================================================================*/
      template<typename T, typename... Args>
        requires std::is_constructible_v<T, Args...>
      Prefab& add_component(Args&&... args);

      /*=================================================================================
      Function Brief:
      Adds a component to this prefab by component type name at runtime.
      Creates a default-constructed instance of the component.

      Parameters:
      component_name - The registered name of the component type.

      Returns:
      True if the component was successfully added, false if the component type
      was not found or could not be added.

      Where to call it:
      When attaching a component to a prefab using a string name (e.g., from UI).
      ==================================================================================*/
      ENGINE_API bool add_component_by_name(const std::string& component_name);

      /*=================================================================================
      Function Brief:
      Removes a component from this prefab.

      Parameters:
      None (template parameter T specifies the component type).

      Where to call it:
      When modifying a prefab's component configuration.
      ==================================================================================*/
      template<typename T>
      Prefab& remove_component();

      /*=================================================================================
      Function Brief:
      Retrieves a component from this prefab.

      Parameters:
      None (template parameter T specifies the component type).

      Where to call it:
      When accessing or modifying a prefab's component data.
      ==================================================================================*/
      template<typename T>
      T* get_component();

      /*=================================================================================
      Function Brief:
      Retrieves a const component from this prefab.

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
      Checks if this prefab handle is valid (not null).

      Parameters:
      None.

      Where to call it:
      Before accessing prefab data.
      ==================================================================================*/
      ENGINE_API bool is_valid() const;

      /*=================================================================================
      Function Brief:
      Allows boolean conversion to check if prefab is valid.

      Parameters:
      None.

      Where to call it:
      In conditional statements.
      ==================================================================================*/
      ENGINE_API operator bool() const;

      /*=================================================================================
      Function Brief:
      Gets the underlying implementation pointer.

      Parameters:
      None.

      Where to call it:
      Internally when accessing implementation details.
      ==================================================================================*/
      ENGINE_API std::shared_ptr<PrefabImpl> get_impl() const;

      /*=================================================================================
      Function Brief:
      Iterates over all components in this prefab.

      Parameters:
      callback - Function to call for each component.

      Where to call it:
      When processing all components in a prefab.
      ==================================================================================*/
      ENGINE_API void for_each_component(std::function<void(const std::string&, IComponentArray*)> callback);

      /*=================================================================================
      Function Brief:
      Iterates over all components in this prefab (const version).

      Parameters:
      callback - Function to call for each component.

      Where to call it:
      When processing all components in a prefab without modification.
      ==================================================================================*/
      ENGINE_API void for_each_component(std::function<void(const std::string&, IComponentArray*)> callback) const;

      /*=================================================================================
      Function Brief:
      Creates a new entity from this prefab, copying all its components.

      Parameters:
      None.

      Where to call it:
      When spawning entities based on this prefab template.
      ==================================================================================*/
      ENGINE_API Entity instantiate() const;

      /*=================================================================================
      Function Brief:
      Adds this prefab to a layer by name.

      Parameters:
      layer_name - Name of the layer to add to.

      Where to call it:
      When configuring which layers the prefab's instantiated entities will be on.
      ==================================================================================*/
      ENGINE_API Prefab& add_to_layer(const std::string& layer_name);

      /*=================================================================================
      Function Brief:
      Removes this prefab from a layer by name.

      Parameters:
      layer_name - Name of the layer to remove from.

      Where to call it:
      When modifying prefab layer configuration.
      ==================================================================================*/
      ENGINE_API Prefab& remove_from_layer(const std::string& layer_name);

      /*=================================================================================
      Function Brief:
      Clears all layers from this prefab.

      Parameters:
      None.

      Where to call it:
      When resetting prefab layer configuration.
      ==================================================================================*/
      ENGINE_API Prefab& clear_layers();

      /*=================================================================================
      Function Brief:
      Gets the layer signature of this prefab.

      Parameters:
      None.

      Where to call it:
      When querying prefab layer configuration.
      ==================================================================================*/
      ENGINE_API LayerSignature get_layers() const;
    };

    /* ====================================== //
    // Prefab functions                       //
    // ====================================== */

    /*=================================================================================
    Function Brief:
    Creates a new empty prefab handle with a unique auto-generated UUID.

    Parameters:
    None.

    Where to call it:
    When defining reusable entity templates.
    ==================================================================================*/
    ENGINE_API Prefab create_prefab();

    /*=================================================================================
    Function Brief:
    Creates a new entity and copies all components from the prefab to it.

    Parameters:
    prefab - The prefab handle to instantiate.

    Where to call it:
    When spawning entities based on a prefab template.
    ==================================================================================*/
    ENGINE_API Entity instantiate_prefab(const Prefab& prefab);

    /*=================================================================================
    Function Brief:
    Creates an entity with a specific ID and copies all components from the prefab to it.
    Used for restoring deleted entities during undo operations.

    Parameters:
    prefab - The prefab handle to instantiate.
    entity_id - The specific entity ID to use.

    Where to call it:
    When restoring a deleted entity during undo operations to preserve entity ID references.
    ==================================================================================*/
    ENGINE_API Entity instantiate_prefab_with_id(const Prefab& prefab, EntityID entity_id);

    /*=================================================================================
    Function Brief:
    Creates a prefab from an existing entity by copying its components.
    The entity automatically becomes an instance of the newly created prefab,
    allowing it to receive future updates when the prefab is modified.

    Parameters:
    entity - The entity to convert to a prefab.

    Where to call it:
    When creating a reusable template from an existing entity.
    ==================================================================================*/
    ENGINE_API Prefab entity_to_prefab(const Entity entity);

    /*=================================================================================
    Function Brief:
    Serializes a prefab to a GenericValue for saving/loading.

    Parameters:
    prefab - The prefab handle to serialize.

    Where to call it:
    When saving prefabs to files or transmitting over network.
    ==================================================================================*/
    ENGINE_API ReflectionLibrary::GenericValue prefab_to_generic_value(const Prefab& prefab);

    /*=================================================================================
    Function Brief:
    Deserializes a GenericValue to create a prefab.

    Parameters:
    value - The GenericValue containing prefab data.

    Where to call it:
    When loading prefabs from files or receiving over network.
    ==================================================================================*/
    ENGINE_API Prefab prefab_from_generic_value(const ReflectionLibrary::GenericValue& value);

    /* ====================================== //
    // Prefab Instance Management             //
    // ====================================== */

    /*=================================================================================
    Function Brief:
    Updates all instances of a prefab with new data from the prefab.
    Finds all entities that were instantiated from the prefab with the given UUID
    and updates their components. Respects per-field overrides - only non-overridden
    fields are updated.

    Parameters:
    prefab_id - UUID of the prefab to update instances for.

    Where to call it:
    After modifying a prefab, to propagate changes to all instances.
    ==================================================================================*/
    ENGINE_API void update_prefab_instances(const std::string& prefab_id);

    /*=================================================================================
    Function Brief:
    Marks a specific field in a component as overridden for an entity.
    Overridden fields will not be updated when the prefab changes.

    Parameters:
    entity - The entity instance.
    type - Component type ID.
    field_path - Dot-separated path to the field (e.g., "color.r").

    Where to call it:
    When a user manually edits a field in the inspector.
    ==================================================================================*/
    ENGINE_API void mark_field_overridden(Entity entity, ComponentTypeID type, const std::string& field_path);

    /*=================================================================================
    Function Brief:
    Removes an override marking from a field, allowing it to receive prefab updates again.

    Parameters:
    entity - The entity instance.
    type - Component type ID.
    field_path - Dot-separated path to the field.

    Where to call it:
    When user clicks "Revert to Prefab" on a field.
    ==================================================================================*/
    ENGINE_API void clear_field_override(Entity entity, ComponentTypeID type, const std::string& field_path);

    /*=================================================================================
    Function Brief:
    Checks if an entity is an instance of a prefab.

    Parameters:
    entity - The entity to check.

    Returns:
    True if the entity is a prefab instance, false otherwise.

    Where to call it:
    In UI code to show prefab-specific options.
    ==================================================================================*/
    ENGINE_API bool is_prefab_instance(Entity entity);

    /*=================================================================================
    Function Brief:
    Binds an existing entity to a prefab by replacing its components/layers with
    the prefab contents and registering prefab-instance metadata.

    Parameters:
    entity - The entity to replace in place.
    prefab - The prefab to bind the entity to.

    Returns:
    True if binding succeeded, false if either input was invalid.

    Where to call it:
    In editor tooling when converting an arbitrary entity into a prefab instance.
    ==================================================================================*/
    ENGINE_API bool bind_entity_to_prefab(Entity entity, const Prefab& prefab);

    /*=================================================================================
    Function Brief:
    Gets the prefab path for a prefab instance entity.

    Parameters:
    entity - The entity instance.

    Returns:
    Prefab path string, or empty string if not a prefab instance.

    Where to call it:
    When displaying which prefab an entity is based on.
    ==================================================================================*/
    ENGINE_API std::string get_prefab_path(Entity entity);

    /*=================================================================================
    Function Brief:
    Unbinds an entity from its source prefab, making it a standalone entity.
    The entity keeps all its current component data but will no longer receive
    updates when the source prefab is modified.

    Parameters:
    entity - The entity to unbind from its prefab.

    Where to call it:
    When a user wants to detach an entity from its prefab in the editor.
    ==================================================================================*/
    ENGINE_API void unbind_prefab_instance(Entity entity);

  } // namespace ECS
} // namespace Engine