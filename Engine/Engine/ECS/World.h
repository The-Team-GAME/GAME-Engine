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
#include "WorldData.h"
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
      requires std::is_constructible_v<T, Args...>
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
    Returns all entities that have all the specified component types and match
    the layer signature (are on at least one of the specified layers).

    Parameters:
    layers - LayerSignature specifying which layers to search.

    Where to call it:
    When you need to find all entities with specific components on specific layers.
    ==================================================================================*/
    template <typename... T>
    std::vector<Entity> get_entities_with(const LayerSignature& layers);

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
    Returns all entities on a specific layer.

    Parameters:
    layer - The LayerID to get entities from.

    Returns:
    std::vector<Entity> - A vector of all entities on the specified layer.

    Where to call it:
    When you need to iterate over entities on a specific layer.
    ==================================================================================*/
    std::vector<Entity> get_entities_on_layer(LayerID layer);

    /*=================================================================================
    Function Brief:
    Returns all entities on a specific layer by name.

    Parameters:
    layer_name - The name of the layer to get entities from.

    Returns:
    std::vector<Entity> - A vector of all entities on the specified layer.

    Where to call it:
    When you need to iterate over entities on a specific layer by name.
    ==================================================================================*/
    std::vector<Entity> get_entities_on_layer(const std::string& layer_name);

    /*=================================================================================
    Function Brief:
    Iterates over all active entities and calls the provided callback function for each.

    Parameters:
    callback - Function to call for each entity. Takes an Entity reference.

    Where to call it:
    When you need to perform an operation on all entities without creating a vector.
    More efficient than get_all_entities() when you just need to iterate.
    ==================================================================================*/
    void for_each_entity(std::function<void(Entity&)> callback);

    /*=================================================================================
    Function Brief:
    Iterates over entities with specific components and calls the callback for each.
    Only entities that have ALL specified component types will be passed to the callback.

    Parameters:
    callback - Function to call for each matching entity. Takes an Entity by value.

    Where to call it:
    When you need to iterate over entities with specific component combinations.
    More efficient than get_entities_with() when you just need to iterate.

    Example:
    for_each_entity_with<Transform, Velocity>([](Entity e) {
      auto* transform = e.get_component<Transform>();
      auto* velocity = e.get_component<Velocity>();
      transform->position += velocity->value;
    });
    ==================================================================================*/
    template <typename... Components>
    void for_each_entity_with(std::function<void(Entity)> callback);

    /*=================================================================================
    Function Brief:
    Iterates over entities on a specific layer and calls the callback for each.

    Parameters:
    layer - The LayerID to iterate over.
    callback - Function to call for each entity on the layer.

    Where to call it:
    When you need to perform operations on entities in a specific layer.
    ==================================================================================*/
    void for_each_entity_on_layer(LayerID layer, std::function<void(Entity&)> callback);

    /*=================================================================================
    Function Brief:
    Iterates over entities on a specific layer (by name) and calls the callback for each.

    Parameters:
    layer_name - The name of the layer to iterate over.
    callback - Function to call for each entity on the layer.

    Where to call it:
    When you need to perform operations on entities in a specific layer by name.
    ==================================================================================*/
    void for_each_entity_on_layer(const std::string& layer_name, std::function<void(Entity&)> callback);

    /*=================================================================================
    Function Brief:
    Iterates over all entities that have the specified component types and are on
    any of the specified layers, calling the callback function for each entity.

    Parameters:
    layers - LayerSignature specifying which layers to search.
    callback - Function to call for each entity: void(Entity entity)

    Where to call it:
    When you need to perform operations on entities with specific components
    on specific layers without creating a temporary vector.

    Example:
    LayerID player_layer = ECS::get_layer_id("player");
    ECS::for_each_entity_with<Components::Health>(
      LayerSignature{player_layer},
      [](Entity entity) {
        auto* health = entity.get_component<Components::Health>();
        // Do something with the health component
      }
    );
    ==================================================================================*/
    template <typename... T>
    void for_each_entity_with(const LayerSignature& layers, std::function<void(Entity)> callback);

    /*=================================================================================
    Function Brief:
    Iterates over pairs of entities where LHS entities are on any layer in the LHS mask
    and RHS entities are on any layer in the RHS mask.
    Calls the callback function for each valid pair (excluding self-pairs).

    Parameters:
    lhs_mask - LayerSignature mask for left-hand side entities
    rhs_mask - LayerSignature mask for right-hand side entities
    callback - Function to call for each pair: void(Entity lhs, Entity rhs)

    Where to call it:
    When you need to check interactions between entities on different layers
    (e.g., player/objects vs bullets for collision detection).

    Example (inline construction):
    LayerID player = ECS::get_layer_id("player");
    LayerID bullet = ECS::get_layer_id("bullet");
    LayerID object = ECS::get_layer_id("object");

    ECS::for_each_entity_pair(
      LayerSignature{player, object},  // LHS: player and object layers
      LayerSignature{bullet},          // RHS: bullet layer
      [](Entity lhs, Entity rhs) {
        // Check collision between lhs and rhs
      }
    );
    ==================================================================================*/
    void for_each_entity_pair(
      const LayerSignature& lhs_mask,
      const LayerSignature& rhs_mask,
      std::function<void(Entity, Entity)> callback
    );    /*=================================================================================
    Function Brief:
    Returns the number of components of type T currently in the world.

    Parameters:
    None (template parameter T specifies the component type).

    Where to call it:
    When you need to know how many entities have a specific component.
    ==================================================================================*/
    template <typename T>
    size_t get_component_count();

    /* =================== //
    // Layer management    //
    // =================== */

    /*=================================================================================
    Function Brief:
    Registers a custom layer with a name and returns its unique LayerID.
    If the layer name already exists, returns the existing LayerID.
    Note: The 'default' layer is always pre-registered with ID 1.

    Parameters:
    name - The name of the layer to register.

    Returns:
    LayerID - The unique ID assigned to this layer.

    Where to call it:
    During initialization to define custom layers for your game.
    ==================================================================================*/
    LayerID register_layer(const std::string& name);

    /*=================================================================================
    Function Brief:
    Retrieves the LayerID associated with a layer name.

    Parameters:
    name - The name of the layer to look up.

    Returns:
    LayerID - The ID of the layer, or INVALID_LAYER if not found.

    Where to call it:
    When you need to get a layer ID from a layer name.
    ==================================================================================*/
    LayerID get_layer_id(const std::string& name);

    /*=================================================================================
    Function Brief:
    Retrieves the name associated with a LayerID.

    Parameters:
    id - The LayerID to look up.

    Returns:
    std::string - The name of the layer, or empty string if not found.

    Where to call it:
    When you need to get a layer name from a layer ID.
    ==================================================================================*/
    std::string get_layer_name(LayerID id);

    /*=================================================================================
    Function Brief:
    Gets the LayerID of the default layer that all entities are automatically
    assigned to on creation. The default layer is initialized during Core startup.

    Parameters:
    None.

    Returns:
    LayerID - The ID of the default layer.

    Where to call it:
    When you need to reference the default layer or check if an entity is on it.
    ==================================================================================*/
    LayerID get_default_layer();

    /*=================================================================================
    Function Brief:
    Adds an entity to one or more layers.

    Parameters:
    entity - The entity to add to layers.

    Where to call it:
    When you want to add an entity to additional layers.
    ==================================================================================*/
    template<LayerID... Layers>
    void add_to_layers(const Entity& entity);

    /*=================================================================================
    Function Brief:
    Removes an entity from one or more layers.

    Parameters:
    entity - The entity to remove from layers.

    Where to call it:
    When you want to remove an entity from specific layers.
    ==================================================================================*/
    template<LayerID... Layers>
    void remove_from_layers(const Entity& entity);

    /*=================================================================================
    Function Brief:
    Sets the exact layers an entity belongs to (replaces existing layers).

    Parameters:
    entity - The entity to set layers for.

    Where to call it:
    When you want to explicitly define which layers an entity is on.
    ==================================================================================*/
    template<LayerID... Layers>
    void set_layers(const Entity& entity);

    /*=================================================================================
    Function Brief:
    Checks if an entity is on a specific layer.

    Parameters:
    entity - The entity to check.
    layer - The layer ID to check for.

    Where to call it:
    When you need to know if an entity is on a specific layer.
    ==================================================================================*/
    bool is_on_layer(const Entity& entity, LayerID layer);

    /*=================================================================================
    Function Brief:
    Checks if an entity is on ANY of the specified layers.

    Parameters:
    entity - The entity to check.

    Where to call it:
    When you need to know if an entity is on at least one of several layers.
    ==================================================================================*/
    template<LayerID... Layers>
    bool is_on_any_layer(const Entity& entity);

    /*=================================================================================
    Function Brief:
    Checks if an entity is on ALL of the specified layers.

    Parameters:
    entity - The entity to check.

    Where to call it:
    When you need to know if an entity is on all specified layers.
    ==================================================================================*/
    template<LayerID... Layers>
    bool is_on_all_layers(const Entity& entity);

    /*=================================================================================
    Function Brief:
    Gets all layers an entity belongs to.

    Parameters:
    entity - The entity to get layers for.

    Where to call it:
    When you need the full layer signature of an entity.
    ==================================================================================*/
    LayerSignature get_entity_layers(const Entity& entity);

    /*=================================================================================
    Function Brief:
    Removes an entity from all layers.

    Parameters:
    entity - The entity to clear layers from.

    Where to call it:
    When you want to remove an entity from all layers.
    ==================================================================================*/
    void clear_layers(const Entity& entity);

    /* =============== //
    // Internal state  //
    // =============== */
    namespace Internal
    {
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

      /*=================================================================================
      Function Brief:
      Internal function that adds an entity to a layer's sparse set.

      Parameters:
      entity - The EntityID to add.
      layer - The LayerID to add the entity to.

      Where to call it:
      Called internally when an entity is added to a layer.
      ==================================================================================*/
      void add_entity_to_layer_set(EntityID entity, LayerID layer);

      /*=================================================================================
      Function Brief:
      Internal function that removes an entity from a layer's sparse set.

      Parameters:
      entity - The EntityID to remove.
      layer - The LayerID to remove the entity from.

      Where to call it:
      Called internally when an entity is removed from a layer.
      ==================================================================================*/
      void remove_entity_from_layer_set(EntityID entity, LayerID layer);
    }
  } // namespace ECS
} // namespace Engine

