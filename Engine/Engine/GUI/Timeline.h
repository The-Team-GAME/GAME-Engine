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
#include "../TypeDef.h"


// Structure to hold the data for an image clip on the timeline
struct TimelineClip {
  s32 id = 0;
  s32 texture_id = 0;
  s32 layer = 0;
  s32 start_frame = 0;
  s32 duration_frames = 30; // Default duration for a new clip
};

class Timeline {

public:
  Timeline();

  void draw();

  s32 get_current_frame() const { return currentFrame; }
  s32 get_num_layers() const { return (s32)layers.size(); }

  // Making playback state public (or adding a getter/setter) for the GameState to use
  bool is_playing = false;
  f64 lastFrameTime = 0.0;

  // Method to add a clip from an external system
  void add_clip(s32 texture_id, s32 layer, s32 start_frame, s32 duration_frames);
  const std::vector<TimelineClip>& get_clips() const { return clips; } //Public getter for clips, needed by TimelineState to check frame existence

  //to select my entity
  void set_selected_entity_id(s32 entity_id) { m_selectedEntityID = entity_id; };

private:
  std::vector<s32> layers;
  s32 currentFrame;
  s32 numFrames;
  f32 zoom;

  ImVec2 cellSize;  // Stores the base cell size (horizontal part is scaled by zoom)
  std::vector<TimelineClip> clips;   //store the placed image clips
  f32 playbackSpeed;   // Playback members

  f64 m_frameAccumulator = 0.0;
  s32 m_nextClipID = 1; //Tracks the ID for the next clip created via DND
  s32 m_activeClipID = 0;       // Clip ID currently being manipulated (0 = none)

  // 0: None, 1: Move, 2: Resize Right, 3: Resize Left
  s32 m_manipulationMode = 0;
  s32 m_dragStartFrame = 0;     // Original start frame on drag/resize start
  s32 m_dragStartDuration = 0;  // Original duration on resize start

  s32 m_selectedEntityID = 0;
};