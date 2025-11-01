//-----------------------------------------------------------------------------------
// file:    Entity.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the Entity class methods for entity handle
//          management, validation, and comparison operations.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Entity.h"
#include "World.h"
#include "WorldData.h"
#include "Reflection.h"

namespace Engine {
  namespace ECS {

    Entity::Entity(EntityID id) : id(id) {}

    EntityID Entity::get_id() const {
      return id;
    }

    bool Entity::is_valid() const {
      return id != INVALID_ENTITY;
    }

    void Entity::destroy() {
      ECS::destroy_entity(*this);
    }

    bool Entity::operator==(const Entity& other) const {
      return id == other.id;
    }

    bool Entity::operator!=(const Entity& other) const {
      return id != other.id;
    }

    void Entity::for_each_component(std::function<void(const std::string&, IComponentArray*)> callback) {
      if (!is_valid())
        return;

      auto& containers = Internal::get_containers();
      auto sig_it = containers.entity_signatures.find(id);

      if (sig_it == containers.entity_signatures.end())
        return;

      const ComponentSignature& signature = sig_it->second;

      // Iterate through all component arrays
      for (auto& [type_id, array] : containers.component_arrays) {
        // Check if this entity has this component type
        if (signature.test(type_id)) {
          std::string type_name = array->get_type_name();
          callback(type_name, array.get());
        }
      }
    }

    ReflectionLibrary::GenericValue entity_to_generic_value(Entity entity) {
      if (!entity.is_valid()) {
        return ReflectionLibrary::GenericValue();
      }

      ReflectionLibrary::GenericValue result;
      result["dataType"] = "Entity";
      result["id"] = static_cast<int>(entity.get_id());

      // Serialize layers and components using helper functions
      result["layers"] = Serialization::serialize_layers(entity.get_layers());
      result["components"] = Serialization::serialize_components(
        [&entity](auto callback) { entity.for_each_component(callback); },
        entity.get_id()
      );

      return result;
    }

    Entity create_entity_from_generic_value(const ReflectionLibrary::GenericValue& value) {
      if (!value.isObject() || !value.has("components") || !value["components"].isArray()) {
        return Entity(INVALID_ENTITY);
      }

      Entity entity = ECS::create_entity();
      EntityID entity_id = entity.get_id();

      auto& containers = Internal::get_containers();

      // Deserialize layers using helper function
      LayerSignature layers = Serialization::deserialize_layers(value);
      entity.clear_layers();
      for (LayerID layer = 0; layer < containers.layer_id_to_name.size(); ++layer) {
        if (layers.test(layer)) {
          entity.add_to_layer(containers.layer_id_to_name[layer]);
        }
      }

      // Deserialize components using helper function
      const auto& components_array = value["components"].getArray();
      Serialization::deserialize_components_to_storage(
        components_array,
        containers.component_arrays,
        containers.entity_signatures[entity_id],
        entity_id
      );

      // Notify systems about the component changes
      Internal::entity_component_changed(entity_id);

      return entity;
    }

    // ============================================================================
    // Layer Management Implementation
    // ============================================================================

    bool Entity::is_on_layer(LayerID layer) const {
      return ECS::is_on_layer(*this, layer);
    }

    LayerSignature Entity::get_layers() const {
      return ECS::get_entity_layers(*this);
    }

    Entity& Entity::clear_layers() {
      ECS::clear_layers(*this);
      return *this;
    }

    Entity& Entity::add_to_layer(const std::string& layer_name) {
      LayerID layer = ECS::get_layer_id(layer_name);
      if (layer != INVALID_LAYER) {
        auto& containers = Internal::get_containers();
        EntityID entity_id = get_id();

        if (!containers.active_entities.contains(entity_id))
          return *this;

        // Only add if not already on this layer
        if (!containers.entity_layers[entity_id].test(layer)) {
          containers.entity_layers[entity_id].set(layer);
          Internal::add_entity_to_layer_set(entity_id, layer);
          Internal::entity_component_changed(entity_id);
        }
      }
      return *this;
    }

    Entity& Entity::remove_from_layer(const std::string& layer_name) {
      LayerID layer = ECS::get_layer_id(layer_name);
      if (layer != INVALID_LAYER) {
        auto& containers = Internal::get_containers();
        EntityID entity_id = get_id();

        if (!containers.active_entities.contains(entity_id))
          return *this;

        // Only remove if currently on this layer
        if (containers.entity_layers[entity_id].test(layer)) {
          containers.entity_layers[entity_id].reset(layer);
          Internal::remove_entity_from_layer_set(entity_id, layer);
          Internal::entity_component_changed(entity_id);
        }
      }
      return *this;
    }

    bool Entity::is_on_layer(const std::string& layer_name) const {
      LayerID layer = ECS::get_layer_id(layer_name);
      if (layer != INVALID_LAYER) {
        return ECS::is_on_layer(*this, layer);
      }
      return false;
    }

  } // namespace ECS
} // namespace Engine