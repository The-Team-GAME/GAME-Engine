#pragma once
#include <vector>
#include <string>
#include <algorithm> // Needed for std::min/max, if used in implementation
#include "imgui.h"

// Structure to hold the data for an image clip on the timeline
struct TimelineClip {
  int textureId;
  int layer;
  int startFrame;
  int durationFrames = 30; // Default duration for a new clip
};

class Timeline {
public:
  Timeline();

  void Draw();

  int GetCurrentFrame() const { return currentFrame; }
  int GetNumLayers() const { return (int)layers.size(); }

  // Making playback state public (or adding a getter/setter) for the GameState to use
  bool isPlaying = false;
  double lastFrameTime = 0.0;

private:
  std::vector<int> layers;
  int currentFrame;
  int numFrames;
  float zoom;


  ImVec2 cellSize;  // Stores the base cell size (horizontal part is scaled by zoom)
  std::vector<TimelineClip> clips;   // New member to store the placed image clips

  float playbackSpeed;   // Playback members
};