#include "World.h"

namespace ECS
{

  Entity World::createEntity()
  {
    EntityID id = nextEntityId++;
    activeEntities.insert(id);
    entitySignatures[id] = ComponentSignature{}; // Initialize empty signature
    return Entity(id, this);
  }

  Entity World::getEntity(EntityID id)
  {
    if (activeEntities.find(id) != activeEntities.end())
    {
      return Entity(id, this);
    }
    return Entity(INVALID_ENTITY, this);
  }

  Entity World::cloneEntity(EntityID sourceId)
  {
    if (!isEntityActive(sourceId))
    {
      return Entity(INVALID_ENTITY, this); // Source entity doesn't exist or is inactive
    }

    Entity newEntity = createEntity();

    // Copy signature from source to new entity
    const auto& sourceSignature = entitySignatures.at(sourceId);
    entitySignatures[newEntity.getId()] = sourceSignature;

    // Copy all components using the signature
    for (auto const& [typeId, componentArray] : componentArrays)
    {
      if (sourceSignature.test(typeId))
      {
        componentArray->cloneComponent(sourceId, newEntity.getId());
      }
    }

    // Notify systems about the new entity
    entityComponentChanged(newEntity.getId());

    return newEntity;
  }

  void World::destroyEntity(EntityID id)
  {
    entitiesToDestroy.insert(id);
  }

  bool World::isEntityActive(EntityID id) const
  {
    return activeEntities.find(id) != activeEntities.end();
  }

  void World::update()
  {
    // Dispatch all queued events
    for (const auto& event : eventQueue)
    {
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
    for (EntityID entityId : entitiesToDestroy)
    {
      if (activeEntities.erase(entityId))
      {
        // Remove from all component arrays
        for (auto& [typeId, array] : componentArrays)
        {
          array->removeEntity(entityId);
        }

        // Remove signature
        entitySignatures.erase(entityId);

        // Remove from all systems
        for (auto& system : systems)
        {
          system->removeEntity(entityId);
        }
      }
    }
    entitiesToDestroy.clear();
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
    result.reserve(activeEntities.size());
    for (EntityID entityId : activeEntities)
    {
      result.emplace_back(entityId, const_cast<World*>(this));
    }
    return result;
  }

  size_t World::getEntityCount() const
  {
    return activeEntities.size();
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
    if (!isEntityActive(entity))
      return false;

    auto it = entitySignatures.find(entity);
    if (it == entitySignatures.end())
      return false;

    const auto& requiredSignature = system.getRequiredSignature();
    const auto& entitySignature = it->second;

    // Fast bitwise check: entity has all required components
    return (requiredSignature & entitySignature) == requiredSignature;
  }

} // namespace ECS