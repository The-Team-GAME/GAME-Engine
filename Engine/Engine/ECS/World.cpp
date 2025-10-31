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

        if (!containers.active_entities.contains(entity))
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

        // Check component requirements
        if ((requiredSignature & entitySignature) != requiredSignature)
          return false;

        // Check layer requirements
        auto layer_it = containers.entity_layers.find(entity);
        if (layer_it != containers.entity_layers.end()) {
          const auto& entityLayers = layer_it->second;
          const auto& systemLayers = system.get_layer_signature();

          // Entity must be on at least one of the system's layers
          if ((entityLayers & systemLayers).none())
            return false;
        }

        return true;
      }

      void add_entity_to_layer_set(EntityID entity, LayerID layer)
      {
        auto& containers = get_containers();
        // Ensure vector is large enough
        if (layer >= containers.layer_entities.size()) {
          containers.layer_entities.resize(layer + 1);
        }
        containers.layer_entities[layer].insert(entity);
      }

      void remove_entity_from_layer_set(EntityID entity, LayerID layer)
      {
        auto& containers = get_containers();
        if (layer < containers.layer_entities.size()) {
          containers.layer_entities[layer].erase(entity);
        }
      }
    } // namespace Internal

    Entity create_entity()
    {
      auto& containers = Internal::get_containers();

      EntityID id = containers.next_entity_id++;
      containers.active_entities.insert(id);
      containers.entity_signatures[id] = ComponentSignature{};
      containers.entity_layers[id] = LayerSignature{};

      // Add entity to default layer
      containers.entity_layers[id].set(DEFAULT_LAYER);
      Internal::add_entity_to_layer_set(id, DEFAULT_LAYER);

      return Entity(id);
    }    Entity get_entity(EntityID id)
    {
      auto& containers = Internal::get_containers();
      if (containers.active_entities.contains(id))
      {
        return Entity(id);
      }
      return Entity(INVALID_ENTITY);
    }

    Entity clone_entity(const Entity& source)
    {
      auto& containers = Internal::get_containers();
      EntityID source_id = source.get_id();
      if (!containers.active_entities.contains(source_id))
      {
        return Entity(INVALID_ENTITY);
      }

      Entity newEntity = create_entity();

      const auto& sourceSignature = containers.entity_signatures.at(source_id);
      containers.entity_signatures[newEntity.get_id()] = sourceSignature;

      // Clone layer signature and add to layer sets
      auto layer_it = containers.entity_layers.find(source_id);
      if (layer_it != containers.entity_layers.end()) {
        const LayerSignature& source_layers = layer_it->second;
        containers.entity_layers[newEntity.get_id()] = source_layers;

        // Add cloned entity to all the source entity's layer sets
        for (LayerID layer = 0; layer < containers.layer_entities.size(); ++layer) {
          if (source_layers.test(layer)) {
            Internal::add_entity_to_layer_set(newEntity.get_id(), layer);
          }
        }
      }

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
      return containers.active_entities.contains(id);
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
        if (containers.active_entities.contains(entityId))
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
        if (containers.active_entities.contains(entityId))
        {
          // Remove entity from all layer sets
          auto layer_it = containers.entity_layers.find(entityId);
          if (layer_it != containers.entity_layers.end()) {
            const LayerSignature& layers = layer_it->second;
            for (LayerID layer = 0; layer < containers.layer_entities.size(); ++layer) {
              if (layers.test(layer)) {
                Internal::remove_entity_from_layer_set(entityId, layer);
              }
            }
          }

          containers.active_entities.erase(entityId);

          for (auto& [typeId, array] : containers.component_arrays)
          {
            array->remove_entity(entityId);
          }

          containers.entity_signatures.erase(entityId);
          containers.entity_layers.erase(entityId);

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

    std::vector<Entity> get_entities_on_layer(LayerID layer)
    {
      auto& containers = Internal::get_containers();
      std::vector<Entity> result;

      if (layer < containers.layer_entities.size()) {
        const auto& layer_set = containers.layer_entities[layer];
        result.reserve(layer_set.size());
        for (EntityID entityId : layer_set) {
          result.emplace_back(entityId);
        }
      }

      return result;
    }

    std::vector<Entity> get_entities_on_layer(const std::string& layer_name)
    {
      LayerID layer = get_layer_id(layer_name);
      if (layer != INVALID_LAYER) {
        return get_entities_on_layer(layer);
      }
      return {};
    }

    // ============================================================================
    // Layer Management Implementation
    // ============================================================================

    LayerID register_layer(const std::string& name)
    {
      auto& containers = Internal::get_containers();

      // Check if layer already exists
      auto it = containers.layer_name_to_id.find(name);
      if (it != containers.layer_name_to_id.end()) {
        return it->second;
      }

      // Ensure we don't exceed MAX_LAYERS
      if (containers.next_layer_id >= MAX_LAYERS) {
        // Return INVALID_LAYER if we've run out of layer slots
        return INVALID_LAYER;
      }

      // Register new layer
      LayerID new_id = containers.next_layer_id++;
      containers.layer_name_to_id[name] = new_id;

      // Resize vectors to include the new layer
      containers.layer_id_to_name.resize(new_id + 1);
      containers.layer_id_to_name[new_id] = name;

      // Resize layer_entities to include the new layer
      containers.layer_entities.resize(new_id + 1);

      return new_id;
    }

    LayerID get_layer_id(const std::string& name)
    {
      auto& containers = Internal::get_containers();
      auto it = containers.layer_name_to_id.find(name);
      if (it != containers.layer_name_to_id.end()) {
        return it->second;
      }
      return INVALID_LAYER;
    }

    std::string get_layer_name(LayerID id)
    {
      auto& containers = Internal::get_containers();
      if (id < containers.layer_id_to_name.size()) {
        return containers.layer_id_to_name[id];
      }
      return "";
    }

    LayerID get_default_layer()
    {
      return DEFAULT_LAYER;
    }

    bool is_on_layer(const Entity& entity, LayerID layer)
    {
      auto& containers = Internal::get_containers();
      EntityID id = entity.get_id();

      auto it = containers.entity_layers.find(id);
      if (it == containers.entity_layers.end())
        return false;

      return it->second.test(layer);
    }

    LayerSignature get_entity_layers(const Entity& entity)
    {
      auto& containers = Internal::get_containers();
      EntityID id = entity.get_id();

      auto it = containers.entity_layers.find(id);
      if (it == containers.entity_layers.end())
        return LayerSignature{};

      return it->second;
    }

    void clear_layers(const Entity& entity)
    {
      auto& containers = Internal::get_containers();
      EntityID id = entity.get_id();

      if (!containers.active_entities.contains(id))
        return;

      // Remove entity from all layer sets
      const LayerSignature& layers = containers.entity_layers[id];
      for (LayerID layer = 0; layer < containers.layer_entities.size(); ++layer) {
        if (layers.test(layer)) {
          Internal::remove_entity_from_layer_set(id, layer);
        }
      }

      containers.entity_layers[id].reset();
      Internal::entity_component_changed(id);
    }

    // ============================================================================
    // Entity Pair Iteration
    // ============================================================================

    void for_each_entity_pair(
      const LayerSignature& lhs_mask,
      const LayerSignature& rhs_mask,
      std::function<void(Entity, Entity)> callback)
    {
      auto& containers = Internal::get_containers();

      // Track processed pairs to avoid duplicates when entities are on multiple layers
      std::unordered_set<EntityPairHash> processed_pairs;

      // Iterate through LHS layers that match the mask
      for (LayerID lhs_layer = 0; lhs_layer < containers.layer_entities.size(); ++lhs_layer) {
        if (!lhs_mask.test(lhs_layer)) continue;

        const auto& lhs_layer_set = containers.layer_entities[lhs_layer];

        // Iterate through RHS layers that match the mask
        for (LayerID rhs_layer = 0; rhs_layer < containers.layer_entities.size(); ++rhs_layer) {
          if (!rhs_mask.test(rhs_layer)) continue;

          const auto& rhs_layer_set = containers.layer_entities[rhs_layer];

          // Iterate all entity pairs between these two layers
          for (EntityID lhs_id : lhs_layer_set) {
            for (EntityID rhs_id : rhs_layer_set) {
              if (lhs_id == rhs_id) continue; // Skip self-pairs

              // Create unique pair ID (order-independent hash)
              EntityPairHash pair_hash = (static_cast<EntityPairHash>(std::min(lhs_id, rhs_id)) << 32) |
                static_cast<EntityPairHash>(std::max(lhs_id, rhs_id));

              // Skip if we've already processed this pair
              if (!processed_pairs.insert(pair_hash).second) continue;

              Entity lhs(lhs_id);
              Entity rhs(rhs_id);
              callback(lhs, rhs);
            }
          }
        }
      }
    }
  } // namespace ECS
} // namespace Engine