//-----------------------------------------------------------------------------------
// file:    TransformAnimationEditorPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the TransformAnimationEditorPanel ImGui interface for
//          visually editing TransformAnimation keyframes with a timeline, curve
//          preview, and easing function selection.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>

namespace Engine::ECS {
  class Entity;
  class Prefab;
}

namespace TransformAnimationEditor {
  void Open(Engine::ECS::Entity entity);
  void Open(Engine::ECS::Prefab prefab, const std::string& prefab_path);
}

void TransformAnimationEditorPanel();
