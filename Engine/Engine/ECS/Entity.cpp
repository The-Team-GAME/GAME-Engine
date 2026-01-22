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
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"

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

    bool Entity::add_component_by_name(const std::string& component_name) {
      if (!is_valid())
        return false;

      auto& containers = Internal::get_containers();

      // Find the component array with matching type name
      for (auto& [type_id, array] : containers.component_arrays) {
        if (array->get_type_name() == component_name) {
          // Check if entity already has this component
          auto sig_it = containers.entity_signatures.find(id);
          if (sig_it != containers.entity_signatures.end() && sig_it->second.test(type_id)) {
            return false; // Already has this component
          }

          // Create a default-constructed component using GenericValue
          ReflectionLibrary::GenericValue default_value = ReflectionLibrary::Object();

          // Add the component
          if (array->add_component_from_generic_value(id, default_value)) {
            containers.entity_signatures[id].set(type_id);
            Internal::entity_component_changed(id);
            return true;
          }
          return false;
        }
      }

      return false; // Component type not found
    }

    ReflectionLibrary::GenericValue entity_to_generic_value(Entity entity) {
      if (!entity.is_valid()) {
        return ReflectionLibrary::GenericValue();
      }

      ReflectionLibrary::GenericValue result;
      // Store the original entity ID for remapping on load
      result["id"] = static_cast<int>(entity.get_id());

      // Serialize layers and components using helper functions
      result["layers"] = Serialization::serialize_layers(entity.get_layers());
      result["components"] = Serialization::serialize_components(
        [&entity](auto callback) { entity.for_each_component(callback); },
        entity.get_id()
      );

      // Serialize prefab instance data if this is a prefab instance
      auto& containers = Internal::get_containers();
      EntityID entity_id = entity.get_id();
      auto prefab_it = containers.prefab_instances.find(entity_id);
      if (prefab_it != containers.prefab_instances.end()) {
        const auto& instance_data = prefab_it->second;

        ReflectionLibrary::GenericValue prefab_ref = ReflectionLibrary::Object();
        prefab_ref["id"] = instance_data.prefab_id;

        // Serialize field overrides
        ReflectionLibrary::GenericValue overrides_obj = ReflectionLibrary::Object();
        for (const auto& [type_id, field_set] : instance_data.field_overrides) {
          ReflectionLibrary::GenericValue fields_array = ReflectionLibrary::Array();
          for (const auto& field_path : field_set) {
            fields_array.push_back(field_path);
          }
          overrides_obj[std::to_string(type_id)] = fields_array;
        }
        prefab_ref["overrides"] = overrides_obj;

        result["prefab_reference"] = prefab_ref;
      }

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
      auto deserialize_result = Serialization::deserialize_components_to_storage(
        components_array,
        containers.component_arrays,
        containers.entity_signatures[entity_id],
        entity_id
      );

      // Check for deserialization errors and warn the user
      if (deserialize_result.has_errors()) {
        std::string error_summary = "Entity loaded with errors:\n";
        for (size_t i = 0; i < deserialize_result.errors.size() && i < 3; ++i) {
          error_summary += "- " + deserialize_result.errors[i] + "\n";
        }
        if (deserialize_result.errors.size() > 3) {
          error_summary += "... and " + std::to_string(deserialize_result.errors.size() - 3) + " more errors";
        }
        Log::warning("{}", error_summary);
        Engine::EventManager::send_event<Engine::Events::ToastEvent>(
          error_summary,
          Engine::Events::ToastType::Warning,
          8.0f
        );
      }

      // Deserialize prefab instance data if present
      if (value.has("prefab_reference") && value["prefab_reference"].isObject()) {
        const auto& prefab_ref = value["prefab_reference"];

        Internal::PrefabInstanceData instance_data;
        if (prefab_ref.has("id") && prefab_ref["id"].isString()) {
          instance_data.prefab_id = prefab_ref["id"].getString();
        }

        // Deserialize field overrides
        if (prefab_ref.has("overrides") && prefab_ref["overrides"].isObject()) {
          const auto& overrides_obj = prefab_ref["overrides"].getObject();
          for (const auto& [type_id_str, fields_val] : overrides_obj) {
            ComponentTypeID type_id = std::stoull(type_id_str);

            if (fields_val.isArray()) {
              for (const auto& field_val : fields_val.getArray()) {
                if (field_val.isString()) {
                  instance_data.field_overrides[type_id].insert(field_val.getString());
                }
              }
            }
          }
        }

        containers.prefab_instances[entity_id] = std::move(instance_data);
      }

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