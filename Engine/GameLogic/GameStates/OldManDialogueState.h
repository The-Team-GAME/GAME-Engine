//-----------------------------------------------------------------------------------
// file:    OldManDialogueState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Overlay scene for the old man dialogue. Pauses gameplay updates from
//          the active scene while rendering dialogue UI in screen space.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"
#include "TipsBookState.h"

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
  void destroy_dimmer();
  void recreate_dimmer();
  void destroy_dialogue_sprite();
  void recreate_dialogue_sprite();

  static constexpr float SCREEN_CENTER_X = 800.0f;
  static constexpr float SCREEN_CENTER_Y = 450.0f;

  // Dialogue sprite dimensions (all slides are 1536x576)
  static constexpr float SLIDE_W = 1536.0f;
  static constexpr float SLIDE_H = 576.0f;
  static constexpr float SLIDE_SCALE = 0.85f; // use smaller scale to avoid oversized dialogue
  static constexpr float SLIDE_X = SCREEN_CENTER_X;                                           // horizontally centred
  static constexpr float SLIDE_Y = 900.0f - (SLIDE_H * SLIDE_SCALE) * 0.5f;                  // anchored to bottom
  static constexpr int   SLIDE_COUNT      = 6;
  static constexpr int   TIPS_PUSH_SLIDE  = 3; // show this slide first (index 3 = slide 4), then open TipsBookState on the next confirm

  static constexpr const char* SLIDE_PATHS[SLIDE_COUNT] = {
    "images/Dialogue/1_Bro_dialogue.png",
    "images/Dialogue/2_Oldman_dialogue.png",
    "images/Dialogue/3_Bro_dialogue.png",
    "images/Dialogue/4_Oldman_dialogue_Tips.png",
    "images/Dialogue/5_Oldman_dialogue.png",
    "images/Dialogue/6_Oldman_dialogue.png",
  };

  Engine::ECS::EntityID dimmer_id_          = 0;
  Engine::ECS::EntityID dialogue_sprite_id_ = 0;
  int   current_slide_              = 0;
  float close_enable_time_unscaled_ = 0.0f;
  bool  e_close_armed_              = false;
  bool  request_tips_on_close_      = false;
  bool  tips_was_open_              = false; // true while TipsBookState is stacked on top
  bool  tips_shown_mid_dialogue_    = false;
};