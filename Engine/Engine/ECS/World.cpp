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
#include "../Perf.h"

namespace Engine {
  namespace ECS
  {
    namespace Internal
    {
      EntityID next_entity_id = 1;
      std::unordered_set<EntityID> active_entities;
      std::unordered_set<EntityID> entities_to_destroy;
      std::unordered_map<EntityID, ComponentSignature> entity_signatures;
      std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>> component_arrays;
      std::vector<std::unique_ptr<System>> systems;

      void entity_component_changed(EntityID entity)
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

      bool system_matches_entity(const System& system, EntityID entity)
      {
        if (!is_entity_active(entity))
          return false;

        auto it = entity_signatures.find(entity);
        if (it == entity_signatures.end())
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
      EntityID id = Internal::next_entity_id++;
      Internal::active_entities.insert(id);
      Internal::entity_signatures[id] = ComponentSignature{};
      return Entity(id);
    }

    Entity get_entity(EntityID id)
    {
      if (Internal::active_entities.find(id) != Internal::active_entities.end())
      {
        return Entity(id);
      }
      return Entity(INVALID_ENTITY);
    }

    Entity clone_entity(const Entity& source)
    {
      EntityID source_id = source.get_id();
      if (!is_entity_active(source))
      {
        return Entity(INVALID_ENTITY);
      }

      Entity newEntity = create_entity();

      const auto& sourceSignature = Internal::entity_signatures.at(source_id);
      Internal::entity_signatures[newEntity.get_id()] = sourceSignature;

      for (auto const& [typeId, componentArray] : Internal::component_arrays)
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
      EntityID id = entity.get_id();
      Internal::entities_to_destroy.insert(id);
    }

    bool is_entity_active(const Entity& entity)
    {
      EntityID id = entity.get_id();
      return Internal::active_entities.find(id) != Internal::active_entities.end();
    }

    void update()
    {
      Engine::Perf::start("ECS Update");

      // Dispatch all queued events to handlers and systems
      EventManager::update_events(Internal::systems);

      // Update all systems
      for (auto& system : Internal::systems)
      {
        system->update();
      }

      // Clean up destroyed entities
      for (EntityID entityId : Internal::entities_to_destroy)
      {
        if (Internal::active_entities.erase(entityId))
        {
          for (auto& [typeId, array] : Internal::component_arrays)
          {
            array->remove_entity(entityId);
          }

          Internal::entity_signatures.erase(entityId);

          for (auto& system : Internal::systems)
          {
            system->remove_entity(entityId);
          }
        }
      }
      Internal::entities_to_destroy.clear();

      Engine::Perf::stop("ECS Update");
    }

    void draw()
    {
      Engine::Perf::start("ECS Draw");

      for (auto& system : Internal::systems)
      {
        system->draw();
      }

      Engine::Perf::stop("ECS Draw");
    }

    std::vector<Entity> get_all_entities()
    {
      std::vector<Entity> result;
      result.reserve(Internal::active_entities.size());
      for (EntityID entityId : Internal::active_entities)
      {
        result.emplace_back(entityId);
      }
      return result;
    }

    size_t get_entity_count()
    {
      return Internal::active_entities.size();
    }
  } // namespace ECS
} // namespace Engine