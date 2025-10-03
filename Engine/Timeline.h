//-----------------------------------------------------------------------------------
// file:    Timeline.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file helps initialise the functions that helps 
//          implements the Timeline class, handling the initialization and
//          rendering of the timeline UI. It manages the grid structure, playback
//          controls, playhead movement, dynamic layer resizing, and processing of
//          drag-and-drop events to place image clips onto the grid.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------


#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include "imgui.h"
#include "TypeDef.h"

// Structure to hold the data for an image clip on the timeline
struct TimelineClip {
  s32 texture_id;
  s32 layer;
  s32 start_frame;
  s32 duration_frames = 30; // Default duration for a new clip
};

class Timeline {
public:
  Timeline();

  void Draw();

  s32 get_current_frame() const { return currentFrame; }
  s32 get_num_layers() const { return (s32)layers.size(); }

  // Making playback state public (or adding a getter/setter) for the GameState to use
  bool isPlaying = false;
  f64 lastFrameTime = 0.0;

private:
  std::vector<s32> layers;
  s32 currentFrame;
  s32 numFrames;
  f32 zoom;


  ImVec2 cellSize;  // Stores the base cell size (horizontal part is scaled by zoom)
  std::vector<TimelineClip> clips;   // New member to store the placed image clips

  f32 playbackSpeed;   // Playback members
};