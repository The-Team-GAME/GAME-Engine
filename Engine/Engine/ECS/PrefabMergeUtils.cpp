//-----------------------------------------------------------------------------------
// file:    PrefabMergeUtils.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of utility functions for merging prefab data with
//          instance overrides at the field level.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "PrefabMergeUtils.h"
#include <sstream>
#include <vector>

namespace Engine {
  namespace ECS {
    namespace PrefabMergeUtils {

      // Helper to split a field path by dots
      static std::vector<std::string> split_path(const std::string& path) {
        std::vector<std::string> parts;
        std::stringstream ss(path);
        std::string part;
        while (std::getline(ss, part, '.')) {
          parts.push_back(part);
        }
        return parts;
      }

      ReflectionLibrary::GenericValue merge_component_with_overrides(
        const ReflectionLibrary::GenericValue& prefab_data,
        const ReflectionLibrary::GenericValue& instance_data,
        const std::unordered_set<std::string>& override_fields
      ) {
        // Start with instance data
        ReflectionLibrary::GenericValue result = instance_data;

        // Get all field paths from prefab
        auto all_fields = get_all_field_paths(prefab_data);

        // Copy non-overridden fields from prefab
        for (const auto& field_path : all_fields) {
          if (override_fields.find(field_path) == override_fields.end()) {
            // Field is not overridden, copy from prefab
            copy_field_by_path(prefab_data, result, field_path);
          }
        }

        return result;
      }

      std::unordered_set<std::string> get_all_field_paths(
        const ReflectionLibrary::GenericValue& value,
        const std::string& prefix
      ) {
        std::unordered_set<std::string> paths;

        if (value.isObject()) {
          for (const auto& [key, val] : value.getObject()) {
            std::string current_path = prefix.empty() ? key : prefix + "." + key;
            paths.insert(current_path);

            // Recursively get nested paths
            auto nested_paths = get_all_field_paths(val, current_path);
            paths.insert(nested_paths.begin(), nested_paths.end());
          }
        }
        else if (value.isArray()) {
          const auto& arr = value.getArray();
          for (size_t i = 0; i < arr.size(); ++i) {
            std::string current_path = prefix + "[" + std::to_string(i) + "]";
            paths.insert(current_path);

            // Recursively get nested paths
            auto nested_paths = get_all_field_paths(arr[i], current_path);
            paths.insert(nested_paths.begin(), nested_paths.end());
          }
        }
        else {
          // Leaf node (primitive value)
          if (!prefix.empty()) {
            paths.insert(prefix);
          }
        }

        return paths;
      }

      bool copy_field_by_path(
        const ReflectionLibrary::GenericValue& source,
        ReflectionLibrary::GenericValue& dest,
        const std::string& field_path
      ) {
        auto parts = split_path(field_path);
        if (parts.empty()) return false;

        // Navigate to the field in source
        const ReflectionLibrary::GenericValue* src_node = &source;
        for (size_t i = 0; i < parts.size() - 1; ++i) {
          const std::string& part = parts[i];
          
          // Handle array indices
          if (part.front() == '[' && part.back() == ']') {
            // Array index
            size_t index = std::stoull(part.substr(1, part.size() - 2));
            if (!src_node->isArray() || index >= src_node->getArray().size()) {
              return false;
            }
            src_node = &src_node->getArray()[index];
          }
          else {
            // Object key
            if (!src_node->isObject() || !src_node->has(part)) {
              return false;
            }
            src_node = &(*src_node)[part];
          }
        }

        // Navigate to the field in dest (create if needed)
        ReflectionLibrary::GenericValue* dest_node = &dest;
        for (size_t i = 0; i < parts.size() - 1; ++i) {
          const std::string& part = parts[i];
          
          // Handle array indices
          if (part.front() == '[' && part.back() == ']') {
            // Array index
            size_t index = std::stoull(part.substr(1, part.size() - 2));
            if (!dest_node->isArray()) {
              return false; // Can't create arrays dynamically
            }
            if (index >= dest_node->getArray().size()) {
              return false;
            }
            dest_node = &(*dest_node)[static_cast<int>(index)];
          }
          else {
            // Object key - create if doesn't exist
            if (!dest_node->isObject()) {
              *dest_node = ReflectionLibrary::Object();
            }
            if (!dest_node->has(part)) {
              (*dest_node)[part] = ReflectionLibrary::GenericValue();
            }
            dest_node = &(*dest_node)[part];
          }
        }

        // Copy the final field value
        const std::string& last_part = parts.back();
        if (last_part.front() == '[' && last_part.back() == ']') {
          // Array index
          size_t index = std::stoull(last_part.substr(1, last_part.size() - 2));
          if (!dest_node->isArray() || index >= dest_node->getArray().size()) {
            return false;
          }
          (*dest_node)[static_cast<int>(index)] = *src_node;
        }
        else {
          // Object key
          if (!src_node->isObject() || !src_node->has(last_part)) {
            // Source is a leaf, copy entire value
            if (!dest_node->isObject()) {
              *dest_node = ReflectionLibrary::Object();
            }
            (*dest_node)[last_part] = *src_node;
          }
          else {
            // Source is object, copy the field
            (*dest_node)[last_part] = (*src_node)[last_part];
          }
        }

        return true;
      }

    } // namespace PrefabMergeUtils
  } // namespace ECS
} // namespace Engine
