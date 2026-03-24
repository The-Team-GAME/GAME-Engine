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
#include <algorithm>

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

    bool Entity::operator==(const Entity other) const {
      return id == other.id;
    }

    bool Entity::operator!=(const Entity other) const {
      return id != other.id;
    }

    void Entity::for_each_component(std::function<void(const std::string&, IComponentArray*)> callback) {
      if (!is_valid())
        return;

      auto& containers = Internal::get_containers();
      if (id >= containers.entity_signatures.size())
        return;

      const ComponentSignature& signature = containers.entity_signatures[id];

      // Iterate through all component arrays
      for (ComponentTypeID type_id = 0; type_id < MAX_COMPONENTS; ++type_id) {
        auto& array = containers.component_arrays[type_id];
        if (!array) continue;
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
      for (ComponentTypeID type_id = 0; type_id < MAX_COMPONENTS; ++type_id) {
        auto& array = containers.component_arrays[type_id];
        if (!array) continue;
        if (array->get_type_name() == component_name) {
          // Check if entity already has this component
          if (id < containers.entity_signatures.size() && containers.entity_signatures[id].test(type_id)) {
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

    Entity Entity::set_name(const std::string& name) {
      ECS::set_entity_name(*this, name);
      return *this;
    }

    const std::string& Entity::get_name() const {
      return ECS::get_entity_name(*this);
    }

    ReflectionLibrary::GenericValue entity_to_generic_value(Entity entity) {
      if (!entity.is_valid()) {
        return ReflectionLibrary::GenericValue();
      }

      ReflectionLibrary::GenericValue result;

      // Serialize stable UUID for merge consistency
      auto& containers = Internal::get_containers();
      EntityID entity_id = entity.get_id();
      auto uuid_it = containers.entity_uuids.find(entity_id);
      if (uuid_it != containers.entity_uuids.end()) {
        result["uuid"] = uuid_it->second;
      }

      // Serialize entity name
      const std::string& name = get_entity_name(entity);
      if (!name.empty()) {
        result["name"] = name;
      }

      // Serialize layers and components using helper functions
      result["layers"] = Serialization::serialize_layers(entity.get_layers());
      result["components"] = Serialization::serialize_components(
        [&entity](auto callback) { entity.for_each_component(callback); },
        entity.get_id()
      );

      // Post-process component data: replace numeric entity ID references in hierarchy
      // components with stable UUID strings so serialized files are ID-independent.
      if (result.has("components") && result["components"].isArray()) {
        for (auto& comp_entry : result.getObject()["components"].getArray()) {
          if (!comp_entry.isObject()) continue;
          auto& comp_obj = comp_entry.getObject();

          auto type_it = comp_obj.find("type");
          auto data_it = comp_obj.find("data");
          if (type_it == comp_obj.end() || data_it == comp_obj.end()) continue;

          const std::string& comp_type = type_it->second.getString();
          auto& data = data_it->second;
          if (!data.isObject()) continue;
          auto& data_obj = data.getObject();

          if (comp_type == "Components::Parent") {
            auto parent_id_it = data_obj.find("parent_id");
            if (parent_id_it != data_obj.end() && parent_id_it->second.isInt()) {
              ECS::EntityID parent_id = static_cast<ECS::EntityID>(parent_id_it->second.getInt());
              if (parent_id != ECS::INVALID_ENTITY) {
                auto pid_uuid_it = containers.entity_uuids.find(parent_id);
                if (pid_uuid_it != containers.entity_uuids.end()) {
                  data_obj["parent_uuid"] = pid_uuid_it->second;
                }
              }
              data_obj.erase("parent_id");
            }
          }
          else if (comp_type == "Components::Children") {
            auto child_ids_it = data_obj.find("child_ids");
            if (child_ids_it != data_obj.end() && child_ids_it->second.isArray()) {
              ReflectionLibrary::GenericValue child_uuids_val = ReflectionLibrary::Array();
              for (const auto& child_val : child_ids_it->second.getArray()) {
                if (!child_val.isInt()) continue;
                ECS::EntityID child_id = static_cast<ECS::EntityID>(child_val.getInt());
                if (child_id == ECS::INVALID_ENTITY) continue;
                auto cid_uuid_it = containers.entity_uuids.find(child_id);
                if (cid_uuid_it != containers.entity_uuids.end()) {
                  child_uuids_val.push_back(cid_uuid_it->second);
                }
              }
              data_obj["child_uuids"] = child_uuids_val;
              data_obj.erase("child_ids");
            }
          }
        }
      }

      // Serialize prefab instance data if this is a prefab instance
      auto prefab_it = containers.prefab_instances.find(entity_id);
      if (prefab_it != containers.prefab_instances.end()) {
        const auto& instance_data = prefab_it->second;

        ReflectionLibrary::GenericValue prefab_ref = ReflectionLibrary::Object();
        prefab_ref["id"] = instance_data.prefab_id;
        if (!instance_data.prefab_filepath.empty()) {
          prefab_ref["filepath"] = instance_data.prefab_filepath;
        }

        // Serialize field overrides using component type names (not numeric IDs)
        // Type names are stable across builds; numeric IDs are runtime-generated and unstable.
        ReflectionLibrary::GenericValue overrides_obj = ReflectionLibrary::Object();
        for (const auto& [type_id, field_set] : instance_data.field_overrides) {
          ReflectionLibrary::GenericValue fields_array = ReflectionLibrary::Array();
          for (const auto& field_path : field_set) {
            fields_array.push_back(field_path);
          }
          // Use the component type name as key for stability
          auto& array = containers.component_arrays[type_id];
          if (array) {
            overrides_obj[array->get_type_name()] = fields_array;
          }
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

      // Deserialize stable UUID if present (for merge consistency).
      // create_entity() already generated a temp UUID and registered it in uuid_to_entity_id;
      // remove that temp entry before overwriting with the persisted UUID.
      if (value.has("uuid") && value["uuid"].isString()) {
        const std::string& temp_uuid = containers.entity_uuids[entity_id];
        containers.uuid_to_entity_id.erase(temp_uuid);
        containers.entity_uuids[entity_id] = value["uuid"].getString();
        containers.uuid_to_entity_id[value["uuid"].getString()] = entity_id;
        Log::info("Loaded entity UUID: {} -> {}", entity_id, value["uuid"].getString());
      }
      else {
        Log::info("No UUID in JSON for entity {}, keeping generated: {}",
          entity_id, containers.entity_uuids[entity_id]);
      }

      // Deserialize entity name if present
      if (value.has("name") && value["name"].isString()) {
        set_entity_name(entity, value["name"].getString());
      }

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
      auto deserialize_result = Serialization::deserialize_components_to_world(
        components_array,
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
        if (prefab_ref.has("filepath") && prefab_ref["filepath"].isString()) {
          instance_data.prefab_filepath = prefab_ref["filepath"].getString();
        }

        // Deserialize field overrides
        // Supports both type-name keys (new format) and numeric type-ID keys (legacy format).
        if (prefab_ref.has("overrides") && prefab_ref["overrides"].isObject()) {
          const auto& overrides_obj = prefab_ref["overrides"].getObject();
          for (const auto& [key_str, fields_val] : overrides_obj) {
            ComponentTypeID type_id = INVALID_COMPONENT_TYPE;

            // Try to parse as a number (legacy format: "13", "20", etc.)
            bool is_numeric = !key_str.empty() && std::all_of(key_str.begin(), key_str.end(), ::isdigit);
            if (is_numeric) {
              type_id = static_cast<ComponentTypeID>(std::stoull(key_str));
            }
            else {
              // New format: key is a component type name (e.g., "Components::Transform2D")
              for (ComponentTypeID tid = 0; tid < MAX_COMPONENTS; ++tid) {
                auto& arr = containers.component_arrays[tid];
                if (arr && arr->get_type_name() == key_str) {
                  type_id = tid;
                  break;
                }
              }
            }

            if (type_id != INVALID_COMPONENT_TYPE && fields_val.isArray()) {
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

    Entity Entity::clear_layers() {
      ECS::clear_layers(*this);
      return *this;
    }

    Entity Entity::add_to_layer(const std::string& layer_name) {
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

    Entity Entity::remove_from_layer(const std::string& layer_name) {
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