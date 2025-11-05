//-----------------------------------------------------------------------------------
// file:    StressTestState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a stress test game state that spawns 1000 animated
//          sprites to test ECS performance. Allows switching between lazy monkey
//          and player sprites via keyboard input.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../../Engine/Systems/PhysicsSystem.h"

class StressTestState : public Engine::Scene
{
private:
  // Textures
  Engine::Graphics::Texture lazy_monkey_texture;
  Engine::Graphics::Texture player_run_texture;

  // Animation parameters for lazy monkey (6 frames, 64x64, 384x64 total)
  const s32 monkey_frame_width = 64;
  const s32 monkey_frame_height = 64;
  const s32 monkey_frame_count = 6;
  const f32 monkey_anim_speed = 0.15f;

  // Animation parameters for player running (8 frames, 64x64)
  const s32 player_frame_width = 64;
  const s32 player_frame_height = 64;
  const s32 player_frame_count = 8;
  const f32 player_anim_speed = 0.1f;

  // Current sprite mode (true = monkey, false = player)
  bool using_monkey_sprite = true;

public:
  void load();
  void init();
  void update();
  void draw();
  void free();
  void unload();
};
