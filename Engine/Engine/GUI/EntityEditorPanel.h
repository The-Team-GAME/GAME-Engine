//-----------------------------------------------------------------------------------
// file:    EntityEditorPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the EntityEditorPanel ImGui interface for runtime
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
  Renders a generic value field using ImGui widgets based on the type of the
  ReflectionLibrary::GenericValue. Supports bool, int, double, string, object,
  and array types, with recursive handling for nested objects. Returns true if
  the value was modified.

  Where to call it:
  Called internally by render_component_from_generic to handle individual fields
  within a component's generic value representation.
  =================================================================================*/
  bool render_generic_value_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value);

  /*=================================================================================
  Function Brief:
  Renders a specialized editor for specific component types, such as Color,
  using ImGui widgets tailored to the component's properties. Returns true if
  the component was rendered.

  Where to call it:
  Called within the component rendering loop in EntityEditorPanel to provide
  custom UI for known component types before falling back to generic rendering.
  =================================================================================*/
  bool render_component_from_generic_specialized(const std::string& component_name, Engine::ECS::Entity& entity);

  /*=================================================================================
  Function Brief:
  Renders a generic component editor using reflection to display and edit
  component fields via ImGui. Handles field rendering, modification detection,
  and writing back changes to the component array. Also provides a remove button.

  Where to call it:
  Called within the component rendering loop in EntityEditorPanel for components
  that do not have specialized rendering, allowing automatic UI generation.
  =================================================================================*/
  bool render_component_from_generic(const std::string& component_name, Engine::ECS::Entity& entity, Engine::ECS::IComponentArray* array);
};

/*=================================================================================
Function Brief:
Creates and renders the main Entity Editor ImGui panel, providing UI for
listing entities, selecting them, adding/removing components, editing component
values, and entity management operations like creation, deletion, cloning,
and exporting.

Where to call it:
Call this function once per frame during the ImGui rendering phase, typically
in the main game loop or UI update function, to display the entity editor.
================================================================================*/
void EntityEditorPanel();
