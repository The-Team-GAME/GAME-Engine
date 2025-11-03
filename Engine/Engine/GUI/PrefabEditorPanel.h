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
