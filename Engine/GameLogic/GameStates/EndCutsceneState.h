//-----------------------------------------------------------------------------------
// file:    EndCutsceneState.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//
// brief:   Plays VictoryCutscene.mpg then EndCutscene.mpg back-to-back before
//          transitioning to CreditsCutsceneState.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class EndCutsceneState : public Engine::Scene
{
private:
  enum class Phase { Victory, End };

  Engine::Video::VideoPlayer player_;
  Phase                      phase_           = Phase::Victory;
  double                     last_video_time_ = -1.0;

  void advance_to_end_video();

public:
  void load();
  void init();
  void update();
  void draw();
  void free();
  void unload();
};
