//-----------------------------------------------------------------------------------
// file:    PrefabEditorPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file declares the PrefabEditorPanel ImGui interface for runtime
//          prefab manipulation. Allows loading, editing, saving, and instantiating
//          prefabs with reflection-based component editing.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include "../Reflection/ReflectionLibrary.h"
#include "../ECS/ECS.h"

namespace PrefabEditorPanelHelpers {
  /*=================================================================================
  Function Brief:
  Renders specialized UI fields for specific component types (e.g., Color, Texture,
  Font). Returns true if the field was handled and rendered, false otherwise.

  Parameters:
  display_name - The name to display for the field in the UI
  field_value  - Reference to the GenericValue containing the field data
  modified     - [out] Set to true if the field value was modified by user input

  Where to call it:
  Called from render_field to attempt specialized rendering before falling back to
  generic.
  ==================================================================================*/
  bool render_specialized_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value, bool& modified);

  /*=================================================================================
  Function Brief:
  Renders generic UI fields based on the field value type (bool, int, float, string,
  object, array).

  Parameters:
  display_name - The name to display for the field in the UI
  field_value  - Reference to the GenericValue containing the field data
  modified     - [out] Set to true if the field value was modified by user input

  Where to call it:
  Called from render_field when specialized rendering doesn't handle the field.
  ==================================================================================*/
  bool render_generic_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value, bool& modified);

  /*=================================================================================
  Function Brief:
  Renders a field using specialized or generic rendering.

  Parameters:
  display_name - The name to display for the field in the UI
  field_value  - Reference to the GenericValue containing the field data
  modified     - [out] Set to true if the field value was modified by user input

  Where to call it:
  When rendering any component field in the prefab editor.
  ==================================================================================*/
  void render_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value, bool& modified);

  /*=================================================================================
  Function Brief:
  Renders a collapsible tree node for a component with all its fields.

  Parameters:
  component_name - The name of the component type
  prefab         - The prefab being edited
  array          - The component array to read/write component data

  Where to call it:
  For each component in the selected prefab.
  ==================================================================================*/
  void render_component_tree(const std::string& component_name, Engine::ECS::Prefab& prefab, Engine::ECS::IComponentArray* array);
};

void PrefabEditorPanel();
