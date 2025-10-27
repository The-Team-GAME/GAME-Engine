//-----------------------------------------------------------------------------------
// file:    World::cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the World namespace methods for entity creation,
//          destruction, cloning, event dispatching, system updates, and entity
//          queries. Manages the main ECS update and draw loops.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "World.h"
#include "WorldData.h"
#include "../Perf.h"

namespace Engine {
  namespace ECS
  {
    namespace Internal
    {
      void entity_component_changed(EntityID entity)
      {
        auto& containers = get_containers();
        for (auto& system : containers.systems)
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

      bool system_matches_entity(const System& system, EntityID entity)
      {
        auto& containers = get_containers();

        if (containers.active_entities.find(entity) == containers.active_entities.end())
          return false;

        auto it = containers.entity_signatures.find(entity);
        if (it == containers.entity_signatures.end())
          return false;

        const auto& entitySignature = it->second;
        const auto& requiredSignature = system.get_required_signature();
        const auto& excludedSignature = system.get_excluded_signature();

        // Check exclusions first (fails fast if excluded components present)
        if ((excludedSignature & entitySignature).any())
          return false;

        // Then check requirements
        return (requiredSignature & entitySignature) == requiredSignature;
      }
    } // namespace Internal

    Entity create_entity()
    {
      auto& containers = Internal::get_containers();
      EntityID id = containers.next_entity_id++;
      containers.active_entities.insert(id);
      containers.entity_signatures[id] = ComponentSignature{};
      return Entity(id);
    }

    Entity get_entity(EntityID id)
    {
      auto& containers = Internal::get_containers();
      if (containers.active_entities.find(id) != containers.active_entities.end())
      {
        return Entity(id);
      }
      return Entity(INVALID_ENTITY);
    }

    Entity clone_entity(const Entity& source)
    {
      auto& containers = Internal::get_containers();
      EntityID source_id = source.get_id();
      if (containers.active_entities.find(source_id) == containers.active_entities.end())
      {
        return Entity(INVALID_ENTITY);
      }

      Entity newEntity = create_entity();

      const auto& sourceSignature = containers.entity_signatures.at(source_id);
      containers.entity_signatures[newEntity.get_id()] = sourceSignature;

      for (auto const& [typeId, componentArray] : containers.component_arrays)
      {
        if (sourceSignature.test(typeId))
        {
          componentArray->clone_component(source_id, newEntity.get_id());
        }
      }

      Internal::entity_component_changed(newEntity.get_id());

      return newEntity;
    }

    void destroy_entity(const Entity& entity)
    {
      auto& containers = Internal::get_containers();
      EntityID id = entity.get_id();
      containers.entities_to_destroy.insert(id);
    }

    bool is_entity_active(const Entity& entity)
    {
      auto& containers = Internal::get_containers();
      EntityID id = entity.get_id();
      return containers.active_entities.find(id) != containers.active_entities.end();
    }

    void update()
    {
      auto& containers = Internal::get_containers();
      Engine::Perf::start("ECS Update");

      // Dispatch all queued events to handlers and systems
      EventManager::update_events(containers.systems);

      // Update all systems
      for (auto& system : containers.systems)
      {
        system->update();
      }

      // Process component removals
      for (const auto& [entityId, typeId] : containers.components_to_remove)
      {
        if (containers.active_entities.find(entityId) != containers.active_entities.end())
        {
          auto array_it = containers.component_arrays.find(typeId);
          if (array_it != containers.component_arrays.end())
          {
            array_it->second->remove_entity(entityId);
            containers.entity_signatures[entityId].reset(typeId);
            Internal::entity_component_changed(entityId);
          }
        }
      }
      containers.components_to_remove.clear();

      // Clean up destroyed entities
      for (EntityID entityId : containers.entities_to_destroy)
      {
        if (containers.active_entities.erase(entityId))
        {
          for (auto& [typeId, array] : containers.component_arrays)
          {
            array->remove_entity(entityId);
          }

          containers.entity_signatures.erase(entityId);

          for (auto& system : containers.systems)
          {
            system->remove_entity(entityId);
          }
        }
      }
      containers.entities_to_destroy.clear();

      Engine::Perf::stop("ECS Update");
    }

    void draw()
    {
      auto& containers = Internal::get_containers();
      Engine::Perf::start("ECS Draw");

      for (auto& system : containers.systems)
      {
        system->draw();
      }

      Engine::Perf::stop("ECS Draw");
    }

    std::vector<Entity> get_all_entities()
    {
      auto& containers = Internal::get_containers();
      std::vector<Entity> result;
      result.reserve(containers.active_entities.size());
      for (EntityID entityId : containers.active_entities)
      {
        result.emplace_back(entityId);
      }
      return result;
    }

    size_t get_entity_count()
    {
      auto& containers = Internal::get_containers();
      return containers.active_entities.size();
    }
  } // namespace ECS
} // namespace Engine