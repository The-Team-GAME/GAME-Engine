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
    entity_signatures[newEntity.getId()] = sourceSignature;

    // Copy all components using the signature
    for (auto const& [typeId, componentArray] : component_arrays)
    {
      if (sourceSignature.test(typeId))
      {
        componentArray->cloneComponent(source_id, newEntity.getId());
      }
    }

    // Notify systems about the new entity
    entityComponentChanged(newEntity.getId());

    return newEntity;
  }

  void World::destory_entity(EntityID id)
  {
    entities_to_destory.insert(id);
  }

  bool World::is_entity_active(EntityID id) const
  {
    return active_entities.find(id) != active_entities.end();
  }

  void World::remove_event_listener(EventTypeID eventType)
  {
    eventHandlers.erase(eventType);
  }

  void World::update()
  {
    // Dispatch all queued events
    for (const auto& event : eventQueue)
    {
      // Call direct event handlers first
      auto handlerIt = eventHandlers.find(event.typeId);
      if (handlerIt != eventHandlers.end()) {
        for (const auto& handler : handlerIt->second) {
          handler(event);
        }
      }

      // Then call system event handlers
      for (auto& system : systems)
      {
        const auto& systemEventSignature = system->getEventSignature();
        if (systemEventSignature.test(event.typeId))
        {
          // Call onEvent for each entity the system manages
          for (EntityID entity : system->getEntities()) {
            system->onEvent(event, entity);
          }
        }
      }
    }
    eventQueue.clear();

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
          array->removeEntity(entityId);
        }

        // Remove signature
        entity_signatures.erase(entityId);

        // Remove from all systems
        for (auto& system : systems)
        {
          system->removeEntity(entityId);
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

  std::vector<Entity> World::getAllEntities() const
  {
    std::vector<Entity> result;
    result.reserve(active_entities.size());
    for (EntityID entityId : active_entities)
    {
      result.emplace_back(entityId, const_cast<World*>(this));
    }
    return result;
  }

  size_t World::getEntityCount() const
  {
    return active_entities.size();
  }

  void World::entityComponentChanged(EntityID entity)
  {
    for (auto& system : systems)
    {
      if (systemMatchesEntity(*system, entity))
      {
        system->addEntity(entity);
      }
      else
      {
        system->removeEntity(entity);
      }
    }
  }

  bool World::systemMatchesEntity(const System& system, EntityID entity) const
  {
    if (!is_entity_active(entity))
      return false;

    auto it = entity_signatures.find(entity);
    if (it == entity_signatures.end())
      return false;

    const auto& requiredSignature = system.getRequiredSignature();
    const auto& entitySignature = it->second;

    // Fast bitwise check: entity has all required components
    return (requiredSignature & entitySignature) == requiredSignature;
  }

} // namespace ECS