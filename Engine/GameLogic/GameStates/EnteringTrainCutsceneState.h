//-----------------------------------------------------------------------------------
// file:    EnteringTrainCutsceneState.h
// project: Monkey Stole My GPA
//
// brief:   Plays the "entering train" cutscene, then returns to the underlying
//          scene (level is opened before the cutscene is pushed).
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class EnteringTrainCutsceneState : public Engine::Scene
{
private:
  Engine::Video::VideoPlayer player;
  std::string next_level_to_open;
  bool open_level_when_done = false;

public:
  bool blocks_lower_draw() const override { return true; }

  void set_next_level_to_open(const std::string& level_path);

  void load();
  void init();
  void update();
  void draw();
  void free();
  void unload();
};
