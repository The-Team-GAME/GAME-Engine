#pragma once

#include "pch.h"

class VideoPlaybackState : public Engine::Scene
{
private:
  Engine::Video::VideoPlayer player;
  f32 accumulated_dt = 0.0f;

public:
  void load();
  void init();
  void update();
  void draw();
  void free();
  void unload();
};
