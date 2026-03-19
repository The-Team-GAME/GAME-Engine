//-----------------------------------------------------------------------------------
// file:    OldManDialogueState.h
// project: Monkey Stole My GPA
//
// brief:   Overlay scene for the old man dialogue. Pauses gameplay updates from
//          the active scene while rendering dialogue UI in screen space.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class OldManDialogueState : public Engine::Scene
{
public:
  inline static bool is_open = false;

  void load()   override;
  void init()   override;
  void update() override;
  void draw()   override;
  void free()   override;
  void unload() override;

private:
  static constexpr float SCREEN_CENTER_X = 800.0f;
  static constexpr float SCREEN_CENTER_Y = 450.0f;

  Engine::ECS::EntityID dimmer_id_ = 0;
  Engine::ECS::EntityID dialogue_bg_id_ = 0;
  Engine::ECS::EntityID npc_portrait_id_ = 0;

  Engine::Graphics::Font dialogue_font_;
  Engine::Graphics::Font continue_font_;

  bool any_input_pressed() const;
};