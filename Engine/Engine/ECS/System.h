//-----------------------------------------------------------------------------------
// file:    System.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the System base class for the ECS. Systems contain
//          game logic and operate on entities with specific component combinations.
//          Supports component requirements, event listening, and entity lifecycle hooks.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "SparseSet.h"
#include "../EventManager/EventManager.h"

namespace Engine {
  namespace ECS {

    class Entity; // Forward declaration

    /* ============================================================= //
    // System base class with clean required components specification //
    // ============================================================= */
    class System {
    protected:
      ComponentSignature required_signature;
      ComponentSignature excluded_signature;
      Engine::EventManager::EventSignature event_signature;
      LayerSignature active_layers;
      SparseSet<EntityID> entities;

    public:
      System();
      virtual ~System();

      /*=================================================================================
      Function Brief:
      Specifies which component types an entity must have for this system to
      process it. Uses variadic templates to accept multiple component types.

      Parameters:
      None (template parameters Components... specify the required component types).

      Where to call it:
      In the system's constructor to define which entities it operates on.
      ==================================================================================*/
      template<typename... Components>
      void require_components();

      /*=================================================================================
      Function Brief:
      Specifies which component types an entity must NOT have for this system to
      process it. Used to exclude entities with certain components.

      Parameters:
      None (template parameters Components... specify the excluded component types).

      Where to call it:
      In the system's constructor when you need to filter out entities with
      specific components.
      ==================================================================================*/
      template<typename... Components>
      void exclude_components();

      /*=================================================================================
      Function Brief:
      Specifies which event types this system should listen to and respond to.

      Parameters:
      None (template parameters Events... specify the event types to listen for).

      Where to call it:
      In the system's constructor to define which events the system handles.
      ==================================================================================*/
      template<typename... Events>
      void listen_to_events();

      /*=================================================================================
      Function Brief:
      Updates all entities managed by this system. Calls update_entity for each
      entity.

      Parameters:
      None.

      Where to call it:
      Called automatically by the ECS during the main update loop.
      ==================================================================================*/
      virtual void update();

      /*=================================================================================
      Function Brief:
      Renders all entities managed by this system. Calls draw_entity for each
      entity.

      Parameters:
      None.

      Where to call it:
      Called automatically by the ECS during the main draw loop.
      ==================================================================================*/
      virtual void draw();

      /*=================================================================================
      Function Brief:
      Handles an event for a specific entity. Override to implement event handling
      logic for this system.

      Parameters:
      event - The event to handle.
      entity - The entity the event is associated with.

      Where to call it:
      Called automatically by the ECS when an event this system listens to is
      dispatched.
      ==================================================================================*/
      virtual void on_event(const Engine::EventManager::Event& event, EntityID entity);

      /*=================================================================================
      Function Brief:
      Updates a single entity. Override to implement per-entity update logic.

      Parameters:
      entity - The entity to update.

      Where to call it:
      Called automatically by the system's update() method for each entity.
      ==================================================================================*/
      virtual void update_entity(Entity& entity);

      /*=================================================================================
      Function Brief:
      Renders a single entity. Override to implement per-entity drawing logic.

      Parameters:
      entity - The entity to draw (const because rendering shouldn't modify entities).

      Where to call it:
      Called automatically by the system's draw() method for each entity.
      ==================================================================================*/
      virtual void draw_entity(const Entity& entity);

      /*=================================================================================
      Function Brief:
      Called when an entity is added to this system (matches the required
      component signature). Override to implement initialization logic.

      Parameters:
      entity - The entity that was added.

      Where to call it:
      Called automatically by the ECS when an entity starts matching the system's
      requirements.
      ==================================================================================*/
      virtual void on_entity_added(const Entity& entity);

      /*=================================================================================
      Function Brief:
      Called when an entity is removed from this system (no longer matches the
      required component signature). Override to implement cleanup logic.

      Parameters:
      entity - The entity that was removed.

      Where to call it:
      Called automatically by the ECS when an entity stops matching the system's
      requirements.
      ==================================================================================*/
      virtual void on_entity_removed(const Entity& entity);

      /*=================================================================================
      Function Brief:
      Internal method to add an entity to this system's tracked entities.

      Parameters:
      entity - The EntityID to add.

      Where to call it:
      Called internally by the ECS when an entity matches this system's requirements.
      ==================================================================================*/
      void add_entity(EntityID entity);

      /*=================================================================================
      Function Brief:
      Internal method to remove an entity from this system's tracked entities.

      Parameters:
      entity - The EntityID to remove.

      Where to call it:
      Called internally by the ECS when an entity no longer matches this system's
      requirements.
      ==================================================================================*/
      void remove_entity(EntityID entity);

      /*=================================================================================
      Function Brief:
      Returns the set of all entities currently tracked by this system.

      Parameters:
      None.

      Where to call it:
      When you need to access the list of entities this system is processing.
      ==================================================================================*/
      const SparseSet<EntityID>& get_entities() const;

      /*=================================================================================
      Function Brief:
      Returns the component signature representing which components entities must
      have for this system.

      Parameters:
      None.

      Where to call it:
      Internally by the ECS to determine if entities match this system.
      ==================================================================================*/
      const ComponentSignature& get_required_signature() const;

      /*=================================================================================
      Function Brief:
      Returns the component signature representing which components entities must
      NOT have for this system.

      Parameters:
      None.

      Where to call it:
      Internally by the ECS to determine if entities match this system.
      ==================================================================================*/
      const ComponentSignature& get_excluded_signature() const;

      /*=================================================================================
      Function Brief:
      Returns the event signature representing which events this system listens to.

      Parameters:
      None.

      Where to call it:
      Internally by the ECS to route events to the appropriate systems.
      ==================================================================================*/
      const Engine::EventManager::EventSignature& get_event_signature() const;

      /*=================================================================================
      Function Brief:
      Specifies which layers this system should process. Systems process entities
      that are on ANY of the specified layers (OR logic).

      Parameters:
      None (template parameters Layers... specify the layer IDs to process).

      Where to call it:
      In the system's constructor to define which layers it operates on.
      ==================================================================================*/
      template<LayerID... Layers>
      void process_layers();

      /*=================================================================================
      Function Brief:
      Returns the layer signature representing which layers this system processes.

      Parameters:
      None.

      Where to call it:
      Internally by the ECS to determine if entities match this system.
      ==================================================================================*/
      const LayerSignature& get_layer_signature() const;
    };

  } // namespace ECS
} // namespace Engine