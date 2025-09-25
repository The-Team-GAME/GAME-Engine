#pragma once
#include <vector>
#include <string>
#include "imgui.h"


class Timeline {
public:
  Timeline();


  void Draw();

  int GetCurrentFrame() const { return currentFrame; }

  int GetNumLayers() const { return (int)layers.size(); }

private:
  std::vector<int> layers;  // Just holds layer indices for now
  int currentFrame;
  int numFrames;            // can be dynamic later
  float zoom;
  ImVec2 cellSize;


  // New members for playback
  bool isPlaying;
  float playbackSpeed;
  double lastFrameTime;
};
