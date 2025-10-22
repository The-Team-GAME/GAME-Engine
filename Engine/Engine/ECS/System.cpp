//-----------------------------------------------------------------------------------
// file:    System.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the System base class methods including entity
//          iteration, event handling, and lifecycle management. Provides default
//          implementations for virtual methods.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "System.h"
#include "Entity.h"

namespace Engine {
  namespace ECS {

    System::~System() = default;

    void System::update() {
      for (EntityID entityId : entities) {
        Entity entity(entityId);
        update_entity(entity);
      }
    }

    void System::draw() {
      for (EntityID entityId : entities) {
        Entity entity(entityId);
        draw_entity(entity);
      }
    }

    void System::on_event([[maybe_unused]] const EventManager::Event& event, [[maybe_unused]] EntityID entity) {}

    void System::update_entity([[maybe_unused]] Entity& entity) {}

    void System::draw_entity([[maybe_unused]] const Entity& entity) {}

    void System::on_entity_added([[maybe_unused]] const Entity& entity) {}

    void System::on_entity_removed([[maybe_unused]] const Entity& entity) {}

    void System::add_entity(EntityID entity) {
      if (entities.insert(entity).second) {
        on_entity_added(Entity(entity));
      }
    }

    void System::remove_entity(EntityID entity) {
      if (entities.erase(entity) > 0) {
        on_entity_removed(Entity(entity));
      }
    }

    const std::unordered_set<EntityID>& System::get_entities() const {
      return entities;
    }

    const ComponentSignature& System::get_required_signature() const {
      return required_signature;
    }

    const ComponentSignature& System::get_excluded_signature() const {
      return excluded_signature;
    }

    const EventManager::EventSignature& System::get_event_signature() const {
      return event_signature;
    }

  } // namespace ECS
} // namespace Engine