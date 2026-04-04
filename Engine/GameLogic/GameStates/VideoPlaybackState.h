//-----------------------------------------------------------------------------------
// file:    VideoPlaybackState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Scene declaration for full-screen video playback.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class VideoPlaybackState : public Engine::Scene
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


