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
