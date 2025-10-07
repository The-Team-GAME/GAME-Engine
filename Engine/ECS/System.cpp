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
#include "World.h"

namespace ECS {

  System::~System() = default;

  void System::update() {
    // Create Entity handles for each entity ID
    for (EntityID entityId : entities) {
      Entity entity(entityId, world);
      update_entity(entity);
    }
  }

  void System::draw() {
    // Create Entity handles for each entity ID
    for (EntityID entityId : entities) {
      const Entity entity(entityId, world);
      draw_entity(entity);
    }
  }

  void System::on_event([[maybe_unused]] const Event& event, [[maybe_unused]] EntityID entity) {}

  void System::update_entity([[maybe_unused]] Entity& entity) {}

  void System::draw_entity([[maybe_unused]] const Entity& entity) {}

  void System::on_entity_added([[maybe_unused]] const Entity& entity) {}

  void System::on_entity_removed([[maybe_unused]] const Entity& entity) {}

  void System::add_entity(EntityID entity) {
    if (entities.insert(entity).second) {
      Entity entityHandle(entity, world);
      on_entity_added(entityHandle);
    }
  }

  void System::remove_entity(EntityID entity) {
    if (entities.erase(entity) > 0) {
      Entity entityHandle(entity, world);
      on_entity_removed(entityHandle);
    }
  }

  const std::unordered_set<EntityID>& System::get_entities() const {
    return entities;
  }

  const ComponentSignature& System::get_required_signature() const {
    return required_signature;
  }

  const EventSignature& System::get_event_signature() const {
    return event_signature;
  }

  void System::set_world(World* w) {
    world = w;
  }

} // namespace ECS