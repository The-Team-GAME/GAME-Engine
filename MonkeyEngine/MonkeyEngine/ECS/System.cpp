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
#include "WorldData.h"
#include <unordered_set>
#include <unordered_map>

namespace Engine {
  namespace ECS {

    System::System() {
      active_layers.set(); // Process all layers by default
    }

    System::~System() = default;

    // TODO: Holiday: DEPRECATE
    void System::update() {
      // Iterate through entities in layer order (lower layer ID = higher priority = first)
      // Update entities at EVERY layer they are on - systems are applied layer by layer
      auto& containers = Internal::get_containers();

      // Reusable buffer to snapshot layer entity IDs (avoids copying full SparseSet)
      static thread_local std::vector<EntityID> iteration_buffer;

      for (LayerID layer = 0; layer < containers.layer_entities.size(); ++layer) {
        // Skip if system doesn't process this layer
        if (!active_layers.test(layer))
          continue;

        // Skip if layer is disabled
        if (!containers.layer_active_state.test(layer))
          continue;

        // Snapshot just the dense keys into a reusable buffer (no SparseSet copy)
        const auto& layer_set = containers.layer_entities[layer];
        iteration_buffer.assign(layer_set.begin(), layer_set.end());

        for (EntityID entityId : iteration_buffer) {
          // Only consider entities in this system's entity set
          if (!entities.contains(entityId)) {
            continue;
          }

          Entity entity(entityId);
          update_entity(entity);
        }
      }
    }

    void System::on_event([[maybe_unused]] const EventManager::Event& event, [[maybe_unused]] EntityID entity) {}

    // TODO: Holiday: DEPRECATE
    void System::draw() {} // empty now, can be overridden by derived systems

    void System::update_entity([[maybe_unused]] Entity entity) {}

    void System::draw_entity([[maybe_unused]] const Entity entity) {}

    void System::on_entity_added([[maybe_unused]] const Entity entity) {}

    void System::on_entity_removed([[maybe_unused]] const Entity entity) {}

    void System::add_entity(EntityID entity) {
      if (!entities.contains(entity)) {
        entities.insert(entity);
        on_entity_added(Entity(entity));
      }
    }

    void System::remove_entity(EntityID entity) {
      if (entities.contains(entity)) {
        entities.erase(entity);
        on_entity_removed(Entity(entity));
      }
    }

    const SparseSet<EntityID>& System::get_entities() const {
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

    const LayerSignature& System::get_layer_signature() const {
      return active_layers;
    }

  } // namespace ECS
} // namespace Engine