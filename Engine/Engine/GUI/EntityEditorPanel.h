//-----------------------------------------------------------------------------------
// file:    EntityEditorPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file declares the EntityEditorPanel ImGui interface for runtime
//          entity and component manipulation. Uses reflection to automatically
//          generate component editor fields.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include "../Reflection/ReflectionLibrary.h"
#include "../ECS/ECS.h"

namespace EntityEditorPanelHelpers {
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
  Renders generic UI fields for basic types (bool, int, double, string) and handles
  nested objects and arrays. Arrays support add, remove, and edit operations for
  all element types including nested objects and arrays.
  Always returns true as it handles all generic cases.

  Parameters:
  display_name - The name to display for the field in the UI
  field_value  - Reference to the GenericValue containing the field data
  modified     - [out] Set to true if the field value was modified by user input

  Where to call it:
  Called from render_field as a fallback after specialized rendering fails.
  ==================================================================================*/
  bool render_generic_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value, bool& modified);


  /*=================================================================================
  Function Brief:
  Attempts to render a field using specialized rendering first, then falls back to
  generic rendering.

  Parameters:
  display_name - The name to display for the field in the UI
  field_value  - Reference to the GenericValue containing the field data
  modified     - [out] Set to true if the field value was modified by user input

  Where to call it:
  Called recursively for nested fields and from render_component_tree to render
  component data.
  ==================================================================================*/
  void render_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value, bool& modified);

  /*=================================================================================
  Function Brief:
  Renders a collapsible tree node for a component, including its fields and a remove
  button. Handles writing back modified data to the component array.

  Parameters:
  component_name - The name of the component type
  entity         - Reference to the entity owning the component
  array          - Pointer to the component array containing the component data

  Where to call it:
  Called from EntityEditorPanel for each reflectable component on the selected entity.
  ==================================================================================*/
  void render_component_tree(const std::string& component_name, Engine::ECS::Entity& entity, Engine::ECS::IComponentArray* array);
};

void EntityEditorPanel();
