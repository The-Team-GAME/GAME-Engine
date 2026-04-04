//-----------------------------------------------------------------------------------
// file:    VictoryCutsceneState.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Plays VictoryCutscene.mpg then transitions to EndCutsceneState.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class FinalCutsceneState : public Engine::Scene
{
private:
  Engine::Video::VideoPlayer player_;
  double                     last_video_time_ = -1.0;

public:
  void load();
  void init();
  void update();
  void draw();
  void free();
  void unload();
};


