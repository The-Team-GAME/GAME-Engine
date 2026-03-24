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
#include "../EngineExport.h"
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

    /**
     * @brief Removes the component array for type T from the world.
     *        Must be called before unloading a DLL that defines T,
     *        so the ComponentArray<T> destructor runs while the vtable is valid.
     */
    template <typename T>
    void unregister_component();

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
    ENGINE_API Entity create_entity();

    /*=================================================================================
    Function Brief:
    Creates an entity with a specific ID. Used for restoring deleted entities
    during undo operations to preserve entity ID references.

    Parameters:
    id - The specific entity ID to use.

    Where to call it:
    When restoring a deleted entity during undo operations.
    ==================================================================================*/
    ENGINE_API Entity create_entity_with_id(EntityID id);

    /*=================================================================================
    Function Brief:
    Retrieves an existing entity by its ID. Returns an invalid entity if the ID
    doesn't correspond to an active entity.

    Parameters:
    id - The EntityID to look up.

    Where to call it:
    When you have an entity ID and need an Entity handle to work with.
    ==================================================================================*/
    ENGINE_API Entity get_entity(EntityID id);

    /*=================================================================================
    Function Brief:
    Creates a duplicate of an entity, copying all its components to a new entity.

    Parameters:
    source - The entity to clone.

    Where to call it:
    When spawning copies of existing entities or implementing duplication features.
    ==================================================================================*/
    ENGINE_API Entity clone_entity(const Entity source);

    /*=================================================================================
    Function Brief:
    Marks an entity for destruction. The entity will be removed at the end of
    the current update frame.

    Parameters:
    entity - The entity to destroy.

    Where to call it:
    When removing game objects or entities from the world.
    ==================================================================================*/
    ENGINE_API void destroy_entity(const Entity entity);

    /*=================================================================================
    Function Brief:
    Marks all active entities for destruction. All entities will be removed at
    the end of the current update frame.

    Parameters:
    None.

    Where to call it:
    When clearing the entire world, transitioning between game states, or resetting
    the game.
    ==================================================================================*/
    ENGINE_API void destroy_all_entities();

    /*=================================================================================
    Function Brief:
    Frees all heap memory held inside the ECS world containers (component arrays,
    entity name strings, layer maps, system objects, etc.).

    Call this AFTER destroy_all_entities() + process_housekeeping() during
    engine shutdown. The leaked WorldContainers* singleton itself is not
    destroyed (by design), but all its internal data is released so the CRT
    leak checker sees clean heap.
    ==================================================================================*/
    ENGINE_API void clear_world();

    /*=================================================================================
    Function Brief:
    Deletes the WorldContainers singleton, freeing all remaining STL structural
    overhead (sentinel nodes, iterator-debug proxies). Call AFTER clear_world()
    and AFTER DLL unload.
    ==================================================================================*/
    ENGINE_API void destroy_world();

    // ============================================================================
    // Multi-World Management
    // ============================================================================

    // Creates a new world with layer/collision config cloned from the active world.
    // Returns the new world's ID. Does NOT set it as active.
    ENGINE_API WorldID create_world();

    // Destroys a non-active world and frees all its data.
    // Returns false if the world doesn't exist or is currently active.
    ENGINE_API bool destroy_world(WorldID id);

    // Switches the active world. Rebuilds system entity sets for the new world.
    // The previous world's entities remain intact but are no longer processed.
    ENGINE_API void set_active_world(WorldID id);

    // Returns the ID of the currently active world.
    ENGINE_API WorldID get_active_world();

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
    ENGINE_API bool is_entity_active(const Entity entity);

    // ============================================================================
    // Entity Activation / Deactivation (for level switching optimization)
    // ============================================================================

    // Deactivates an entity: removes it from active_entities and layer sets,
    // but preserves all component data, signatures, layers, names, and prefab data.
    // Deactivated entities are invisible to all systems, queries, and iteration.
    ENGINE_API void deactivate_entity(EntityID id);

    // Reactivates a previously deactivated entity: re-inserts it into active_entities
    // and layer sets, and re-registers it with matching systems.
    ENGINE_API void activate_entity(EntityID id);

    // Returns true if the entity is currently deactivated (alive but suspended).
    ENGINE_API bool is_entity_deactivated(EntityID id);

    /* =================== //
    // Entity naming        //
    // =================== */

    /*=================================================================================
    Function Brief:
    Sets the display name for an entity.

    Parameters:
    entity - The entity to name.
    name   - The display name string.

    Where to call it:
    When creating entities or renaming them in the editor.
    ==================================================================================*/
    ENGINE_API void set_entity_name(const Entity entity, const std::string& name);

    /*=================================================================================
    Function Brief:
    Gets the display name for an entity. Returns "Entity {id}" if no name is set.

    Parameters:
    entity - The entity to query.

    Where to call it:
    When displaying entity names in the editor or debug output.
    ==================================================================================*/
    ENGINE_API const std::string& get_entity_name(const Entity entity);

    /* =================== //
    // Entity UUID         //
    // =================== */

    // Returns the UUID string for the given entity. Returns empty string if entity is invalid.
    ENGINE_API const std::string& get_entity_uuid(const Entity entity);

    // Looks up an entity by its UUID. Returns an invalid entity if not found.
    // O(1) lookup via the uuid_to_entity_id reverse map.
    ENGINE_API Entity find_entity_by_uuid(const std::string& uuid);

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
    void add_component(const Entity entity, Args &&...args);

    /*=================================================================================
    Function Brief:
    Removes a component of type T from an entity.

    Parameters:
    entity - The entity to remove the component from.

    Where to call it:
    When detaching a component from an entity.
    ==================================================================================*/
    template <typename T>
    void remove_component(const Entity entity);

    /*=================================================================================
    Function Brief:
    Retrieves a pointer to the component of type T on an entity.

    Parameters:
    entity - The entity to get the component from.

    Where to call it:
    When you need to access or modify a component's data.
    ==================================================================================*/
    template <typename T>
    T* get_component(const Entity entity);

    /*=================================================================================
    Function Brief:
    Checks if an entity has all the specified component types.

    Parameters:
    entity - The entity to check.

    Where to call it:
    Before accessing multiple components to verify they all exist.
    ==================================================================================*/
    template <typename... T>
    bool has_components(const Entity entity);

    /*=================================================================================
    Function Brief:
    Checks if an entity has a component of type T.

    Parameters:
    entity - The entity to check.

    Where to call it:
    Before accessing a component to verify it exists.
    ==================================================================================*/
    template <typename T>
    bool has_component(const Entity entity);

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
    ENGINE_API void remove_event_listener(Engine::EventManager::EventTypeID eventType);

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
    ENGINE_API void update();

    /*=================================================================================
    Function Brief:
    Updates only game systems (SystemType::Game). Skipped when game is paused.
    Also processes queued events.

    Parameters:
    None.

    Where to call it:
    In the main game loop, only when the game is not paused.
    ==================================================================================*/
    ENGINE_API void update_game();

    /*=================================================================================
    Function Brief:
    Updates only editor systems (SystemType::Editor). Always runs, even when paused.
    These include gizmo manipulation, camera control, etc.

    Parameters:
    None.

    Where to call it:
    In the main game loop, every frame regardless of pause state.
    ==================================================================================*/
    ENGINE_API void update_editor();

    /*=================================================================================
    Function Brief:
    Requests a one-shot update for a specific system type. The system will be
    updated once during the next update_editor() call, even if it's a Game system
    and the game is paused. Useful for editor scrubbing/previewing.

    Parameters:
    None (template parameter T specifies the system type to update).

    Where to call it:
    In editor code when you need to trigger a system update while paused.
    ==================================================================================*/
    template<typename T>
    void request_system_update();

    /*=================================================================================
    Function Brief:
    Requests a one-shot update for a specific entity. All systems that track this
    entity will process it once during the next update_editor() call, even if
    the game is paused. Useful for editor scrubbing/previewing.

    Parameters:
    entity - The EntityID to update.

    Where to call it:
    In editor code when you need to trigger an entity update while paused.
    ==================================================================================*/
    ENGINE_API void request_entity_update(EntityID entity);

    /*=================================================================================
    Function Brief:
    Processes pending entity deletions and component modifications. Called every frame
    even when physics is paused to ensure ECS state changes are applied.

    Parameters:
    None.

    Where to call it:
    In the main game loop, called every frame regardless of pause state.
    ==================================================================================*/
    ENGINE_API void process_housekeeping();

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
    ENGINE_API void draw();

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
    ENGINE_API std::vector<Entity> get_all_entities();

    /*=================================================================================
    Function Brief:
    Returns the total number of active entities in the world.

    Parameters:
    None.

    Where to call it:
    When you need to know how many entities exist.
    ==================================================================================*/
    ENGINE_API size_t get_entity_count();

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
    ENGINE_API std::vector<Entity> get_entities_on_layer(LayerID layer);

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
    ENGINE_API std::vector<Entity> get_entities_on_layer(const std::string& layer_name);

    /*=================================================================================
    Function Brief:
    Iterates over all active entities and calls the provided callback function for each.

    Parameters:
    callback - Function to call for each entity. Takes an Entity reference.

    Where to call it:
    When you need to perform an operation on all entities without creating a vector.
    More efficient than get_all_entities() when you just need to iterate.
    ==================================================================================*/
    ENGINE_API void for_each_entity(std::function<void(Entity)> callback);

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
    template <typename... Components, typename Func>
    void for_each_entity_with(Func&& callback);

    /*=================================================================================
    Function Brief:
    Iterates over entities with specific components and passes the components directly
    to the callback. This is a more convenient version that avoids repeated get_component
    calls inside the callback.

    Parameters:
    callback - Function that receives the Entity and component pointers directly.

    Where to call it:
    When you need to iterate over entities and immediately work with their components.
    More convenient than for_each_entity_with when you need component access.

    Example (single component):
    for_each<Transform>([](Entity e, Transform* t) {
      t->position.x += 1.0f;
    });

    Example (multiple components):
    for_each<Transform, Velocity>([](Entity e, Transform* t, Velocity* v) {
      t->position += v->value * dt;
    });

    Example (without entity):
    for_each<Transform, Velocity>([](Transform* t, Velocity* v) {
      t->position += v->value * dt;
    });
    ==================================================================================*/
    template <typename... Components, typename Func>
    void for_each(Func&& callback);

    /*=================================================================================
    Function Brief:
    Iterates over entities on a specific layer and calls the callback for each.

    Parameters:
    layer - The LayerID to iterate over.
    callback - Function to call for each entity on the layer.

    Where to call it:
    When you need to perform operations on entities in a specific layer.
    ==================================================================================*/
    ENGINE_API void for_each_entity_on_layer(LayerID layer, std::function<void(Entity)> callback);

    /*=================================================================================
    Function Brief:
    Iterates over entities on a specific layer (by name) and calls the callback for each.

    Parameters:
    layer_name - The name of the layer to iterate over.
    callback - Function to call for each entity on the layer.

    Where to call it:
    When you need to perform operations on entities in a specific layer by name.
    ==================================================================================*/
    ENGINE_API void for_each_entity_on_layer(const std::string& layer_name, std::function<void(Entity)> callback);

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
    template <typename... T, typename Func>
    void for_each_entity_with(const LayerSignature& layers, Func&& callback);

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
    );

    /*=================================================================================
    Function Brief:
    Iterates over pairs of entities where both LHS and RHS entities have all the
    specified components. Calls the callback function for each valid pair
    (excluding self-pairs).

    Parameters:
    Components... - Component types that both entities must have
    callback - Function to call for each pair: void(Entity lhs, Entity rhs)

    Where to call it:
    When you need to check interactions between entities with specific components
    (e.g., physics bodies colliding with other physics bodies).

    Example:
    ECS::for_each_entity_pair<Transform, Collider>([](Entity lhs, Entity rhs) {
      // Both lhs and rhs are guaranteed to have Transform and Collider
      auto* t1 = lhs.get_component<Transform>();
      auto* t2 = rhs.get_component<Transform>();
      auto* c1 = lhs.get_component<Collider>();
      auto* c2 = rhs.get_component<Collider>();
      // Check collision between lhs and rhs
    });
    ==================================================================================*/
    template <typename... Components, typename Func>
    void for_each_entity_pair(Func&& callback);

    /*=================================================================================
    Function Brief:
    Iterates over pairs of entities where both LHS and RHS entities have all the
    specified components AND match the layer masks. LHS entities must be on any
    layer in the LHS mask, and RHS entities must be on any layer in the RHS mask.
    Calls the callback function for each valid pair (excluding self-pairs).

    Parameters:
    Components... - Component types that both entities must have
    lhs_mask - LayerSignature mask for left-hand side entities
    rhs_mask - LayerSignature mask for right-hand side entities
    callback - Function to call for each pair: void(Entity lhs, Entity rhs)

    Where to call it:
    When you need to check interactions between entities with specific components
    on specific layers (e.g., player bullets vs enemy colliders).

    Example:
    LayerID player_layer = ECS::get_layer_id("player");
    LayerID enemy_layer = ECS::get_layer_id("enemy");

    ECS::for_each_entity_pair<Transform, Collider>(
      LayerSignature{player_layer},  // LHS: player layer
      LayerSignature{enemy_layer},   // RHS: enemy layer
      [](Entity lhs, Entity rhs) {
        // lhs is on player layer with Transform and Collider
        // rhs is on enemy layer with Transform and Collider
        auto* t1 = lhs.get_component<Transform>();
        auto* t2 = rhs.get_component<Transform>();
        auto* c1 = lhs.get_component<Collider>();
        auto* c2 = rhs.get_component<Collider>();
        // Check collision between lhs and rhs
      }
    );
    ==================================================================================*/
    template <typename... Components, typename Func>
    void for_each_entity_pair(
      const LayerSignature& lhs_mask,
      const LayerSignature& rhs_mask,
      Func&& callback
    );

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
    ENGINE_API LayerID register_layer(const std::string& name);

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
    ENGINE_API LayerID get_layer_id(const std::string& name);

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
    ENGINE_API std::string get_layer_name(LayerID id);

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
    ENGINE_API LayerID get_default_layer();

    /*=================================================================================
    Function Brief:
    Enables a layer, allowing systems to process entities on it.

    Parameters:
    layer - The LayerID to enable.

    Where to call it:
    When you want to activate a layer that was previously disabled.
    ==================================================================================*/
    ENGINE_API void enable_layer(LayerID layer);

    /*=================================================================================
    Function Brief:
    Disables a layer, preventing systems from processing entities on it.

    Parameters:
    layer - The LayerID to disable.

    Where to call it:
    When you want to temporarily pause/hide entities on a specific layer.
    ==================================================================================*/
    ENGINE_API void disable_layer(LayerID layer);

    /*=================================================================================
    Function Brief:
    Enables a layer by name, allowing systems to process entities on it.

    Parameters:
    layer_name - The name of the layer to enable.

    Where to call it:
    When you want to activate a layer using its string name.
    ==================================================================================*/
    ENGINE_API void enable_layer(const std::string& layer_name);

    /*=================================================================================
    Function Brief:
    Disables a layer by name, preventing systems from processing entities on it.

    Parameters:
    layer_name - The name of the layer to disable.

    Where to call it:
    When you want to temporarily pause/hide entities on a specific layer by name.
    ==================================================================================*/
    ENGINE_API void disable_layer(const std::string& layer_name);

    /*=================================================================================
    Function Brief:
    Checks if a layer is currently active/enabled.

    Parameters:
    layer - The LayerID to check.

    Returns:
    bool - True if the layer is active, false otherwise.

    Where to call it:
    When you need to know if a layer is currently being processed.
    ==================================================================================*/
    ENGINE_API bool is_layer_active(LayerID layer);

    /*=================================================================================
    Function Brief:
    Checks if a layer is currently active/enabled by name.

    Parameters:
    layer_name - The name of the layer to check.

    Returns:
    bool - True if the layer is active, false otherwise.

    Where to call it:
    When you need to know if a layer is currently being processed.
    ==================================================================================*/
    ENGINE_API bool is_layer_active(const std::string& layer_name);

    /*=================================================================================
    Function Brief:
    Adds an entity to one or more layers.

    Parameters:
    entity - The entity to add to layers.

    Where to call it:
    When you want to add an entity to additional layers.
    ==================================================================================*/
    template<LayerID... Layers>
    void add_to_layers(const Entity entity);

    /*=================================================================================
    Function Brief:
    Removes an entity from one or more layers.

    Parameters:
    entity - The entity to remove from layers.

    Where to call it:
    When you want to remove an entity from specific layers.
    ==================================================================================*/
    template<LayerID... Layers>
    void remove_from_layers(const Entity entity);

    /*=================================================================================
    Function Brief:
    Sets the exact layers an entity belongs to (replaces existing layers).

    Parameters:
    entity - The entity to set layers for.

    Where to call it:
    When you want to explicitly define which layers an entity is on.
    ==================================================================================*/
    template<LayerID... Layers>
    void set_layers(const Entity entity);

    /*=================================================================================
    Function Brief:
    Checks if an entity is on a specific layer.

    Parameters:
    entity - The entity to check.
    layer - The layer ID to check for.

    Where to call it:
    When you need to know if an entity is on a specific layer.
    ==================================================================================*/
    ENGINE_API bool is_on_layer(const Entity entity, LayerID layer);

    /*=================================================================================
    Function Brief:
    Checks if an entity is on ANY of the specified layers.

    Parameters:
    entity - The entity to check.

    Where to call it:
    When you need to know if an entity is on at least one of several layers.
    ==================================================================================*/
    template<LayerID... Layers>
    bool is_on_any_layer(const Entity entity);

    /*=================================================================================
    Function Brief:
    Checks if an entity is on ALL of the specified layers.

    Parameters:
    entity - The entity to check.

    Where to call it:
    When you need to know if an entity is on all specified layers.
    ==================================================================================*/
    template<LayerID... Layers>
    bool is_on_all_layers(const Entity entity);

    /*=================================================================================
    Function Brief:
    Gets all layers an entity belongs to.

    Parameters:
    entity - The entity to get layers for.

    Where to call it:
    When you need the full layer signature of an entity.
    ==================================================================================*/
    ENGINE_API LayerSignature get_entity_layers(const Entity entity);

    /*=================================================================================
    Function Brief:
    Removes an entity from all layers.

    Parameters:
    entity - The entity to clear layers from.

    Where to call it:
    When you want to remove an entity from all layers.
    ==================================================================================*/
    ENGINE_API void clear_layers(const Entity entity);

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
      ENGINE_API void entity_component_changed(EntityID entity);

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
      ENGINE_API bool system_matches_entity(const System& system, EntityID entity);

      /*=================================================================================
      Function Brief:
      Internal function that adds an entity to a layer's sparse set.

      Parameters:
      entity - The EntityID to add.
      layer - The LayerID to add the entity to.

      Where to call it:
      Called internally when an entity is added to a layer.
      ==================================================================================*/
      ENGINE_API void add_entity_to_layer_set(EntityID entity, LayerID layer);

      /*=================================================================================
      Function Brief:
      Internal function that removes an entity from a layer's sparse set.

      Parameters:
      entity - The EntityID to remove.
      layer - The LayerID to remove the entity from.

      Where to call it:
      Called internally when an entity is removed from a layer.
      ==================================================================================*/
      ENGINE_API void remove_entity_from_layer_set(EntityID entity, LayerID layer);
    }
  } // namespace ECS
} // namespace Engine

