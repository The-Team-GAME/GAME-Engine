//-----------------------------------------------------------------------------------
// file:    PrefabMergeUtils.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Utility functions for merging prefab data with instance overrides.
//          Provides field-level manipulation of GenericValue objects to support
//          per-field override tracking in prefab instances.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Reflection/ReflectionLibrary.h"
#include "Types.h"
#include <string>
#include <unordered_set>

namespace Engine {
  namespace ECS {
    namespace PrefabMergeUtils {

      /*=================================================================================
      Function Brief:
      Merges prefab component data with instance component data, respecting field overrides.
      Only non-overridden fields are copied from the prefab.

      Parameters:
      prefab_data - GenericValue containing the prefab's component data.
      instance_data - GenericValue containing the instance's component data.
      override_fields - Set of field paths that are overridden in the instance.

      Returns:
      GenericValue with merged data (instance overrides) + (prefab non-overrides).

      Where to call it:
      When updating a prefab instance to apply prefab changes while preserving customizations.
      ==================================================================================*/
      ReflectionLibrary::GenericValue merge_component_with_overrides(
        const ReflectionLibrary::GenericValue& prefab_data,
        const ReflectionLibrary::GenericValue& instance_data,
        const std::unordered_set<std::string>& override_fields
      );

      /*=================================================================================
      Function Brief:
      Recursively collects all field paths in a GenericValue.
      For nested objects, creates paths like "color.r", "color.g", etc.

      Parameters:
      value - GenericValue to extract field paths from.
      prefix - Current path prefix for recursive traversal.

      Returns:
      Set of all field paths in the GenericValue.

      Where to call it:
      When needing to enumerate all possible field paths in a component.
      ==================================================================================*/
      std::unordered_set<std::string> get_all_field_paths(
        const ReflectionLibrary::GenericValue& value,
        const std::string& prefix = ""
      );

      /*=================================================================================
      Function Brief:
      Copies a specific field value from source to destination GenericValue by field path.
      Handles nested field paths like "transform.position.x".

      Parameters:
      source - GenericValue to copy from.
      dest - GenericValue to copy to (modified in place).
      field_path - Dot-separated path to the field (e.g., "color.r").

      Returns:
      True if the field was successfully copied, false if field path was not found.

      Where to call it:
      When applying prefab updates field-by-field during instance merge.
      ==================================================================================*/
      bool copy_field_by_path(
        const ReflectionLibrary::GenericValue& source,
        ReflectionLibrary::GenericValue& dest,
        const std::string& field_path
      );

    } // namespace PrefabMergeUtils
  } // namespace ECS
} // namespace Engine
