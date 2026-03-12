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

      // Helper to check if a field path or any of its parent paths is overridden
      static bool is_field_overridden(
        const std::string& field_path,
        const std::unordered_set<std::string>& override_fields
      ) {
        // Direct match
        if (override_fields.find(field_path) != override_fields.end()) {
          return true;
        }

        // Check if any parent path is overridden
        // e.g., if "position" is overridden, then "position.x" is also considered overridden
        std::string::size_type pos = field_path.find('.');
        while (pos != std::string::npos) {
          std::string parent_path = field_path.substr(0, pos);
          if (override_fields.find(parent_path) != override_fields.end()) {
            return true;
          }
          pos = field_path.find('.', pos + 1);
        }

        return false;
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
          if (!is_field_overridden(field_path, override_fields)) {
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

        // For simple top-level fields
        if (parts.size() == 1) {
          const std::string& field_name = parts[0];
          if (source.isObject() && source.has(field_name)) {
            if (!dest.isObject()) {
              dest = ReflectionLibrary::Object();
            }
            dest[field_name] = source[field_name];
            return true;
          }
          return false;
        }

        // For nested fields, we need to navigate and copy
        // This is simplified - only handles direct field access, not deep nesting
        // For now, just copy the top-level field that contains the nested data
        const std::string& top_field = parts[0];
        if (source.isObject() && source.has(top_field)) {
          if (!dest.isObject()) {
            dest = ReflectionLibrary::Object();
          }
          dest[top_field] = source[top_field];
          return true;
        }

        return false;
      }

    } // namespace PrefabMergeUtils
  } // namespace ECS
} // namespace Engine
