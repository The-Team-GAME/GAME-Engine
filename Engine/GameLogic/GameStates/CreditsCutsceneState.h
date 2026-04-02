//-----------------------------------------------------------------------------------
// file:    CreditsCutsceneState.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//
// brief:   Plays the ending/credits cutscene video before transitioning back to
//          the main menu.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class CreditsCutsceneState : public Engine::Scene
{
private:
  Engine::Video::VideoPlayer player;
  double last_video_time_ = -1.0;
  float fade_alpha_       = 0.0f;

public:
  void load();
  void init();
  void update();
  void draw();
  void free();
  void unload();
};
