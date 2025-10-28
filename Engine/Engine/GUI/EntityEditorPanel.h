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
  bool render_generic(const std::string& display_name, ReflectionLibrary::GenericValue& field_value, bool& modified);
  bool render_specialized(const std::string& display_name, ReflectionLibrary::GenericValue& field_value, bool& modified);
  bool render_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value);
  void render_component_from_generic(const std::string& component_name, Engine::ECS::Entity& entity, Engine::ECS::IComponentArray* array);
};

void EntityEditorPanel();
