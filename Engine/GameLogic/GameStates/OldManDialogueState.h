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
  inline static bool open_tips_request = false;
  inline static bool suppress_oldman_reopen_once = false;
  inline static float block_oldman_reopen_until_unscaled = 0.0f;

  static bool consume_open_tips_request() {
    bool requested = open_tips_request;
    open_tips_request = false;
    return requested;
  }

  static bool consume_suppress_oldman_reopen_once() {
    bool requested = suppress_oldman_reopen_once;
    suppress_oldman_reopen_once = false;
    return requested;
  }

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
  float close_enable_time_unscaled_ = 0.0f;
  bool e_close_armed_ = false;
  bool request_tips_on_close_ = false;
};