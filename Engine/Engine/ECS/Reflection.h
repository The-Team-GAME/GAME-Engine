//-----------------------------------------------------------------------------------
// file:    Reflection.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file contains helper functions for serializing and deserializing
//          ECS entities and prefabs to/from GenericValue representations.
//          Provides reusable serialization logic for layers and components.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "ComponentArray.h"
#include "WorldData.h"
#include "../Reflection/ReflectionLibrary.h"
#include "../Logger.h"
#include <unordered_map>
#include <memory>

namespace Engine {
  namespace ECS {
    namespace Serialization {

      // ============================================================================
      // Layer Serialization
      // ============================================================================

      /*=================================================================================
      Function Brief:
      Serializes a LayerSignature to a JSON array of layer name strings.

      Parameters:
      - layers: The LayerSignature to serialize.

      Returns:
      A ReflectionLibrary::Array containing layer names as strings.
      ==================================================================================*/
      inline ReflectionLibrary::Array serialize_layers(const LayerSignature& layers) {
        ReflectionLibrary::Array layers_array;
        auto& containers = Internal::get_containers();

        for (LayerID layer = 0; layer < containers.layer_id_to_name.size(); ++layer) {
          if (layers.test(layer)) {
            layers_array.push_back(containers.layer_id_to_name[layer]);
          }
        }
        return layers_array;
      }

      /*=================================================================================
      Function Brief:
      Deserializes a LayerSignature from a JSON value containing a "layers" array.

      Parameters:
      - value: The GenericValue containing the "layers" array field.

      Returns:
      A LayerSignature with the appropriate layer bits set.
      ==================================================================================*/
      inline LayerSignature deserialize_layers(const ReflectionLibrary::GenericValue& value) {
        LayerSignature result;
        if (value.has("layers") && value["layers"].isArray()) {
          const auto& layers_array = value["layers"].getArray();
          for (const auto& layer_value : layers_array) {
            if (layer_value.isString()) {
              LayerID layer = get_layer_id(layer_value.getString());
              if (layer != INVALID_LAYER) {
                result.set(layer);
              }
            }
          }
        }
        return result;
      }

      // ============================================================================
      // Component Serialization
      // ============================================================================

      /*=================================================================================
      Function Brief:
      Serializes components to a JSON array. Uses a callback to iterate through
      components and converts each reflectable component to a GenericValue.

      Parameters:
      - for_each_component_callback: A callable that takes a callback function
        accepting (const std::string& type_name, IComponentArray* array).
      - entity_id: The entity ID to use for component serialization (use 0 for prefabs).

      Returns:
      A ReflectionLibrary::Array containing component entries with "type" and "data".
      ==================================================================================*/
      template<typename CallbackType>
      inline ReflectionLibrary::Array serialize_components(
        CallbackType for_each_component_callback,
        EntityID entity_id
      ) {
        ReflectionLibrary::Array components_array;

        for_each_component_callback([&](const std::string& type_name, IComponentArray* array) {
          if (!array->is_reflectable()) {
            return;
          }

          ReflectionLibrary::GenericValue component_entry;
          component_entry["type"] = type_name;
          component_entry["data"] = array->to_generic_value(entity_id);

          components_array.push_back(component_entry);
          });

        return components_array;
      }

      /*=================================================================================
      Function Brief:
      Result structure for component deserialization, containing any errors encountered.
      ==================================================================================*/
      struct DeserializeResult {
        std::vector<std::string> errors;
        bool has_errors() const { return !errors.empty(); }
      };

      /*=================================================================================
      Function Brief:
      Deserializes components from a JSON array and adds them to the specified storage.
      Creates component arrays as needed and updates the component signature.

      Parameters:
      - components_array: The JSON array containing component entries.
      - storage: The component storage map to add components to (supports std::unordered_map or Memory::HashMap).
      - signature: The component signature to update.
      - entity_id: The entity ID to use for component deserialization (use 0 for prefabs).

      Returns:
      A DeserializeResult containing any errors encountered during deserialization.
      ==================================================================================*/
      template <typename StorageMap>
      inline DeserializeResult deserialize_components_to_storage(
        const ReflectionLibrary::Array& components_array,
        StorageMap& storage,
        ComponentSignature& signature,
        EntityID entity_id
      ) {
        DeserializeResult result;
        auto& containers = Internal::get_containers();

        for (const auto& component_entry : components_array) {
          if (!component_entry.isObject() || !component_entry.has("type") || !component_entry.has("data")) {
            result.errors.push_back("Invalid component entry: missing 'type' or 'data' field");
            continue;
          }

          const std::string& component_type = component_entry["type"].getString();
          const auto& component_data = component_entry["data"];

          if (component_data.isNull()) {
            Log::warning("Deserializing component '{}' with null data.", component_type);
          }

          // Find matching component array type in world
          bool found = false;
          for (auto& [type_id, world_array] : containers.component_arrays) {
            if (world_array->get_type_name() == component_type) {
              found = true;
              // Create storage if it doesn't exist
              if (storage.find(type_id) == storage.end()) {
                storage[type_id] = world_array->create_empty();
              }

              // Add component to storage and check for errors
              if (!storage[type_id]->add_component_from_generic_value(entity_id, component_data)) {
                std::string error = "Failed to deserialize component '" + component_type + "': data may be corrupted or component structure has changed";
                Log::warning("{}", error);
                result.errors.push_back(error);
              }
              else {
                // Update signature only on success
                signature.set(type_id);
              }

              break;
            }
          }

          if (!found) {
            std::string error = "No registered component type found for '" + component_type + "' during deserialization";
            Log::warning("{}", error);
            result.errors.push_back(error);
          }

        }
        return result;
      }

    } // namespace Serialization
  } // namespace ECS
} // namespace Engine
