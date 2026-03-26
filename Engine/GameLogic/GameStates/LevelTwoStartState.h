//-----------------------------------------------------------------------------------
// file:    LevelTwoStartState.h
// project: Monkey Stole My GPA
//
// brief:   Starts LevelTwo in GameplayState.
//          This is intended as a manual entry point to LevelTwo (no portal required).
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class LevelTwoStartState : public Engine::Scene
{
public:
  bool blocks_lower_draw() const override { return true; }

  void load();
  void init();
  void update();
  void draw();
  void free();
  void unload();
};
