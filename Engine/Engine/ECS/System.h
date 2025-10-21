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
#include "../EventManager/EventManager.h"
#include <unordered_set>

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
      std::unordered_set<EntityID> entities;

    public:
      virtual ~System();

      virtual void update();
      virtual void draw();

      virtual void on_event([[maybe_unused]] const EventManager::Event& event, [[maybe_unused]] EntityID entity);

      virtual void update_entity([[maybe_unused]] Entity& entity);
      virtual void draw_entity([[maybe_unused]] const Entity& entity);

      virtual void on_entity_added([[maybe_unused]] const Entity& entity);
      virtual void on_entity_removed([[maybe_unused]] const Entity& entity);

      template<typename... Components>
      void require_components();

      template<typename... Components>
      void exclude_components();

      template<typename... Events>
      void listen_to_events();

      void add_entity(EntityID entity);
      void remove_entity(EntityID entity);

      const ComponentSignature& get_required_signature() const { return required_signature; }
      const ComponentSignature& get_excluded_signature() const { return excluded_signature; }
      const Engine::EventManager::EventSignature& get_event_signature() const { return event_signature; }
      const std::unordered_set<EntityID>& get_entities() const { return entities; }
    };

  } // namespace ECS
} // namespace Engine