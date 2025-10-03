//-----------------------------------------------------------------------------------
// file:    World.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the World class methods for entity creation,
//          destruction, cloning, event dispatching, system updates, and entity
//          queries. Manages the main ECS update and draw loops.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "World.h"

namespace ECS
{

  Entity World::create_entity()
  {
    EntityID id = next_entity_id++;
    active_entities.insert(id);
    entity_signatures[id] = ComponentSignature{}; // Initialize empty signature
    return Entity(id, this);
  }

  Entity World::get_entity(EntityID id)
  {
    if (active_entities.find(id) != active_entities.end())
    {
      return Entity(id, this);
    }
    return Entity(INVALID_ENTITY, this);
  }

  Entity World::clone_entity(EntityID source_id)
  {
    if (!is_entity_active(source_id))
    {
      return Entity(INVALID_ENTITY, this); // Source entity doesn't exist or is inactive
    }

    Entity newEntity = create_entity();

    // Copy signature from source to new entity
    const auto& sourceSignature = entity_signatures.at(source_id);
    entity_signatures[newEntity.get_id()] = sourceSignature;

    // Copy all components using the signature
    for (auto const& [typeId, componentArray] : component_arrays)
    {
      if (sourceSignature.test(typeId))
      {
        componentArray->clone_component(source_id, newEntity.get_id());
      }
    }

    // Notify systems about the new entity
    entity_component_changed(newEntity.get_id());

    return newEntity;
  }

  void World::destroy_entity(EntityID id)
  {
    entities_to_destory.insert(id);
  }

  bool World::is_entity_active(EntityID id) const
  {
    return active_entities.find(id) != active_entities.end();
  }

  void World::remove_event_listener(EventTypeID eventType)
  {
    event_handlers.erase(eventType);
  }

  void World::update()
  {
    // Dispatch all queued events
    for (const auto& event : event_queue)
    {
      // Call direct event handlers first
      auto handlerIt = event_handlers.find(event.type_id);
      if (handlerIt != event_handlers.end()) {
        for (const auto& handler : handlerIt->second) {
          handler(event);
        }
      }

      // Then call system event handlers
      for (auto& system : systems)
      {
        const auto& systemEventSignature = system->get_event_signature();
        if (systemEventSignature.test(event.type_id))
        {
          // Call on_event for each entity the system manages
          for (EntityID entity : system->get_entities()) {
            system->on_event(event, entity);
          }
        }
      }
    }
    event_queue.clear();

    // Update all systems
    for (auto& system : systems)
    {
      system->update();
    }

    // Clean up destroyed entities
    for (EntityID entityId : entities_to_destory)
    {
      if (active_entities.erase(entityId))
      {
        // Remove from all component arrays
        for (auto& [typeId, array] : component_arrays)
        {
          array->remove_entity(entityId);
        }

        // Remove signature
        entity_signatures.erase(entityId);

        // Remove from all systems
        for (auto& system : systems)
        {
          system->remove_entity(entityId);
        }
      }
    }
    entities_to_destory.clear();
  }

  void World::draw()
  {
    // Draw all systems
    for (auto& system : systems)
    {
      system->draw();
    }
  }

  std::vector<Entity> World::get_all_entities() const
  {
    std::vector<Entity> result;
    result.reserve(active_entities.size());
    for (EntityID entityId : active_entities)
    {
      result.emplace_back(entityId, const_cast<World*>(this));
    }
    return result;
  }

  size_t World::get_entity_count() const
  {
    return active_entities.size();
  }

  void World::entity_component_changed(EntityID entity)
  {
    for (auto& system : systems)
    {
      if (system_matches_entity(*system, entity))
      {
        system->add_entity(entity);
      }
      else
      {
        system->remove_entity(entity);
      }
    }
  }

  bool World::system_matches_entity(const System& system, EntityID entity) const
  {
    if (!is_entity_active(entity))
      return false;

    auto it = entity_signatures.find(entity);
    if (it == entity_signatures.end())
      return false;

    const auto& requiredSignature = system.get_required_signature();
    const auto& entitySignature = it->second;

    // Fast bitwise check: entity has all required components
    return (requiredSignature & entitySignature) == requiredSignature;
  }

} // namespace ECS