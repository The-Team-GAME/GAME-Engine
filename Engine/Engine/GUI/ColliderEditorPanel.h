//-----------------------------------------------------------------------------------
// file:    ColliderEditorPanel.h
// project: Monkey Stole My GPA
//
// author:  Antigravity
//
// brief:   This file defines the ColliderEditorPanel ImGui interface for visualizing
//          and editing entity colliders.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>

namespace Engine::ECS {
  class Entity;
  class Prefab;
}

namespace ColliderEditor {
  void Open(Engine::ECS::Entity entity);
  void Open(Engine::ECS::Prefab prefab, const std::string& prefab_path);
  void OpenForState(Engine::ECS::Entity entity, const std::string& state_name);
  bool IsOpen();
  void Close();
}

void ColliderEditorPanel();
