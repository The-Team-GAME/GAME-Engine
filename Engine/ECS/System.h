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
#include "ComponentManager.h"
#include "EventManager.h"
#include <unordered_set>

namespace ECS {

  class World; // Forward declaration
  class Entity; // Forward declaration

  /* ============================================================= //
  // System base class with clean required components specification //
  // ============================================================= */
  class System {
  protected:
    World* world = nullptr;
    std::unordered_set<EntityID> entities;
    ComponentSignature required_signature;
    EventSignature event_signature;

  public:
    virtual ~System();

    // Clean way to specify required components!
    template<typename... Components>
    void require_components();

    // Clean way to specify listened-to events!
    template<typename... Events>
    void listen_to_events();

    // System logic
    virtual void update();

    // Drawing/rendering logic
    virtual void draw();

    // Override for handling a specific event for a specific entity
    virtual void on_event(const Event& event, EntityID entity);

    // Override for per-entity logic
    virtual void update_entity(Entity& entity);

    // Override for per-entity drawing logic (const reference because rendering shouldn't modify entities)
    virtual void draw_entity(const Entity& entity);

    // Override for entity lifecycle events
    virtual void on_entity_added(const Entity& entity);
    virtual void on_entity_removed(const Entity& entity);

    // Internal methods
    void add_entity(EntityID entity);
    void remove_entity(EntityID entity);

    const std::unordered_set<EntityID>& get_entities() const;
    const ComponentSignature& get_required_signature() const;
    const EventSignature& get_event_signature() const;

  private:
    friend class World;
    void set_world(World* w);
  };

} // namespace ECS

