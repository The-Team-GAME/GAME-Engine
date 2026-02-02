//-----------------------------------------------------------------------------------
// file:    AnimatorEditorPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the AnimatorEditorPanel ImGui interface for
//          visually editing Animator components with multiple tracks, keyframes,
//          and easing function selection for any component property.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>

namespace Engine::ECS {
  class Entity;
  class Prefab;
}

namespace AnimatorEditor {
  void Open(Engine::ECS::Entity entity);
  void Open(Engine::ECS::Prefab prefab, const std::string& prefab_path);
}

void AnimatorEditorPanel();
