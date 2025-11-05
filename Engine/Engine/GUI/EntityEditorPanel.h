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
