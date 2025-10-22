//-----------------------------------------------------------------------------------
// file:    World.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the World namespace which is the main ECS container.
//          Manages entities, components, systems, and events. Provides the primary
//          interface for creating entities, adding components, registering systems,
//          and dispatching events.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "ComponentArray.h"
#include "Entity.h"
#include "System.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>

namespace Engine {
  namespace ECS
  {
    /* ========================= //
    // Component registration    //
    // ========================= */

    /*=================================================================================
    Function Brief:
    Registers a component type with the ECS, creating storage for components of
    type T. Called automatically when needed.

    Parameters:
    None (template parameter T specifies the component type to register).

    Where to call it:
    Usually called automatically by the ECS. Can be called manually to pre-register
    component types.
    ==================================================================================*/
    template <typename T>
    void register_component();

    /*=================================================================================
    Function Brief:
    Retrieves the component array that stores all components of type T. Creates
    the array if it doesn't exist.

    Parameters:
    None (template parameter T specifies the component type).

    Where to call it:
    Internally by the ECS when accessing component storage.
    ==================================================================================*/
    template <typename T>
    ComponentArray<T>* get_component_array();

    /* =================== //
    // Entity management   //
    // =================== */

    /*=================================================================================
    Function Brief:
    Creates a new entity with a unique ID and empty component signature.

    Parameters:
    None.

    Where to call it:
    When spawning a new game object or entity in the world.
    ==================================================================================*/
    Entity create_entity();

    /*=================================================================================
    Function Brief:
    Retrieves an existing entity by its ID. Returns an invalid entity if the ID
    doesn't correspond to an active entity.

    Parameters:
    id - The EntityID to look up.

    Where to call it:
    When you have an entity ID and need an Entity handle to work with.
    ==================================================================================*/
    Entity get_entity(EntityID id);

    /*=================================================================================
    Function Brief:
    Creates a duplicate of an entity, copying all its components to a new entity.

    Parameters:
    source - The entity to clone.

    Where to call it:
    When spawning copies of existing entities or implementing duplication features.
    ==================================================================================*/
    Entity clone_entity(const Entity& source);

    /*=================================================================================
    Function Brief:
    Marks an entity for destruction. The entity will be removed at the end of
    the current update frame.

    Parameters:
    entity - The entity to destroy.

    Where to call it:
    When removing game objects or entities from the world.
    ==================================================================================*/
    void destroy_entity(const Entity& entity);

    /*=================================================================================
    Function Brief:
    Destroys all entities that have the specified component types.

    Parameters:
    None (template parameters T... specify the component types to match).

    Where to call it:
    When you need to clear all entities with specific components (e.g., cleanup).
    ==================================================================================*/
    template <typename... T>
    void destroy_entities_with();

    /*=================================================================================
    Function Brief:
    Checks if an entity is currently active in the world.

    Parameters:
    entity - The entity to check.

    Where to call it:
    Before performing operations on an entity to ensure it hasn't been destroyed.
    ==================================================================================*/
    bool is_entity_active(const Entity& entity);

    /* ====================== //
    // Component management   //
    // ====================== */

    /*=================================================================================
    Function Brief:
    Adds a component of type T to an entity, constructing it with the provided
    arguments.

    Parameters:
    entity - The entity to add the component to.
    args - Constructor arguments for the component.

    Where to call it:
    When attaching a new component to an entity.
    ==================================================================================*/
    template <typename T, typename... Args>
    void add_component(const Entity& entity, Args &&...args);

    /*=================================================================================
    Function Brief:
    Removes a component of type T from an entity.

    Parameters:
    entity - The entity to remove the component from.

    Where to call it:
    When detaching a component from an entity.
    ==================================================================================*/
    template <typename T>
    void remove_component(const Entity& entity);

    /*=================================================================================
    Function Brief:
    Retrieves a pointer to the component of type T on an entity.

    Parameters:
    entity - The entity to get the component from.

    Where to call it:
    When you need to access or modify a component's data.
    ==================================================================================*/
    template <typename T>
    T* get_component(const Entity& entity);

    /*=================================================================================
    Function Brief:
    Checks if an entity has all the specified component types.

    Parameters:
    entity - The entity to check.

    Where to call it:
    Before accessing multiple components to verify they all exist.
    ==================================================================================*/
    template <typename... T>
    bool has_components(const Entity& entity);

    /*=================================================================================
    Function Brief:
    Checks if an entity has a component of type T.

    Parameters:
    entity - The entity to check.

    Where to call it:
    Before accessing a component to verify it exists.
    ==================================================================================*/
    template <typename T>
    bool has_component(const Entity& entity);

    /* ================== //
    // System management  //
    // ================== */

    /*=================================================================================
    Function Brief:
    Creates and registers a system of type T with the ECS, constructing it with
    the provided arguments.

    Parameters:
    args - Constructor arguments for the system.

    Where to call it:
    During initialization to set up the game's systems.
    ==================================================================================*/
    template <typename T, typename... Args>
    T* add_system(Args &&...args);

    /*=================================================================================
    Function Brief:
    Removes a system of type T from the ECS.

    Parameters:
    None (template parameter T specifies the system type to remove).

    Where to call it:
    When disabling or removing game systems.
    ==================================================================================*/
    template <typename T>
    void remove_system();

    /* ================= //
    // Event management  //
    // ================= */

    /*=================================================================================
    Function Brief:
    Sends an event to all listening systems. The event is queued and processed
    at the end of the current update frame.

    Parameters:
    args - Constructor arguments for the event data.

    Where to call it:
    When triggering game events (collisions, input, state changes, etc.).
    ==================================================================================*/
    template <typename T, typename... Args>
    void send_event(Args&&... args);

    /*=================================================================================
    Function Brief:
    Registers a handler function to be called when events of type T are dispatched.
    Handler receives the typed event data.

    Parameters:
    handler - Function to call when the event occurs.

    Where to call it:
    When setting up event listeners for specific event types.
    ==================================================================================*/
    template <typename T>
    void add_event_listener(std::function<void(const T&)> handler);

    /*=================================================================================
    Function Brief:
    Registers a handler function to be called when events of type T are dispatched.
    Handler receives the generic Event wrapper.

    Parameters:
    handler - Function to call when the event occurs.

    Where to call it:
    When setting up event listeners that need access to the Event wrapper.
    ==================================================================================*/
    template <typename T>
    void add_event_listener(std::function<void(const Engine::EventManager::Event&)> handler);

    /*=================================================================================
    Function Brief:
    Removes all event listeners for a specific event type ID.

    Parameters:
    eventType - The EventTypeID to remove listeners for.

    Where to call it:
    When cleaning up event listeners.
    ==================================================================================*/
    void remove_event_listener(Engine::EventManager::EventTypeID eventType);

    /*=================================================================================
    Function Brief:
    Removes all event listeners for event type T.

    Parameters:
    None (template parameter T specifies the event type).

    Where to call it:
    When cleaning up event listeners for a specific event type.
    ==================================================================================*/
    template <typename T>
    void remove_event_listener();

    /* ==================== //
    // Main update loop     //
    // ==================== */

    /*=================================================================================
    Function Brief:
    Updates all registered systems and processes queued events. Call this once
    per frame.

    Parameters:
    None.

    Where to call it:
    In the main game loop, once per frame.
    ==================================================================================*/
    void update();

    /* ================ //
    // Main draw loop   //
    // ================ */

    /*=================================================================================
    Function Brief:
    Calls the draw method on all registered systems. Call this once per frame
    during the rendering phase.

    Parameters:
    None.

    Where to call it:
    In the main game loop, during the rendering phase.
    ==================================================================================*/
    void draw();

    /* ============================================== //
    // Query methods - return Entity instead of EntityID //
    // ============================================== */

    /*=================================================================================
    Function Brief:
    Returns all entities that have all the specified component types.

    Parameters:
    None (template parameters T... specify the required component types).

    Where to call it:
    When you need to find all entities with a specific component combination.
    ==================================================================================*/
    template <typename... T>
    std::vector<Entity> get_entities_with();

    /*=================================================================================
    Function Brief:
    Returns the first entity found that has all the specified component types.

    Parameters:
    None (template parameters T... specify the required component types).

    Where to call it:
    When you need to find a single entity with specific components.
    ==================================================================================*/
    template <typename... T>
    Entity get_first_entity_with();

    /*=================================================================================
    Function Brief:
    Returns all currently active entities in the world.

    Parameters:
    None.

    Where to call it:
    When you need to iterate over or inspect all entities.
    ==================================================================================*/
    std::vector<Entity> get_all_entities();

    /*=================================================================================
    Function Brief:
    Returns the total number of active entities in the world.

    Parameters:
    None.

    Where to call it:
    When you need to know how many entities exist.
    ==================================================================================*/
    size_t get_entity_count();

    /*=================================================================================
    Function Brief:
    Returns the number of components of type T currently in the world.

    Parameters:
    None (template parameter T specifies the component type).

    Where to call it:
    When you need to know how many entities have a specific component.
    ==================================================================================*/
    template <typename T>
    size_t get_component_count();

    /* =============== //
    // Internal state  //
    // =============== */
    namespace Internal
    {
      extern EntityID next_entity_id;
      extern std::unordered_set<EntityID> active_entities;
      extern std::unordered_set<EntityID> entities_to_destroy;
      extern std::unordered_map<EntityID, ComponentSignature> entity_signatures;
      extern std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>> component_arrays;
      extern std::vector<std::unique_ptr<System>> systems;

      /*=================================================================================
      Function Brief:
      Internal function called when an entity's components change. Updates system
      memberships based on the new component signature.

      Parameters:
      entity - The EntityID whose components changed.

      Where to call it:
      Called internally by the ECS after adding or removing components.
      ==================================================================================*/
      void entity_component_changed(EntityID entity);

      /*=================================================================================
      Function Brief:
      Internal function that checks if a system's requirements match an entity's
      components. Considers both required and excluded components.

      Parameters:
      system - The system to check.
      entity - The EntityID to check against.

      Where to call it:
      Called internally by the ECS when determining system membership.
      ==================================================================================*/
      bool system_matches_entity(const System& system, EntityID entity);
    }
  } // namespace ECS
} // namespace Engine

