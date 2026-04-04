//-----------------------------------------------------------------------------------
// file:    EnteringTrainCutsceneState.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Plays the "entering train" cutscene, then returns to the underlying
//          scene and opens the next level when the cutscene finishes.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class EnteringTrainCutsceneState : public Engine::Scene
{
private:
  Engine::Video::VideoPlayer player;
  std::string next_level_to_open;
  bool open_level_when_done = false;

  double last_video_time_ = -1.0;

public:

  void set_next_level_to_open(const std::string& level_path);

  void load();
  void init();
  void update();
  void draw();
  void free();
  void unload();
};

