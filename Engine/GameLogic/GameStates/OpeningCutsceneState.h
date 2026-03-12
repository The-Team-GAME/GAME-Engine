//-----------------------------------------------------------------------------------
// file:    OpeningCutsceneState.h
// project: Monkey Stole My GPA
//
// brief:   Plays the opening cutscene before transitioning to gameplay.
//          Automatically loads GameplayState when the video ends.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class OpeningCutsceneState : public Engine::Scene
{
private:
  Engine::Video::VideoPlayer player;

public:
  void load();
  void init();
  void update();
  void draw();
  void free();
  void unload();
};
