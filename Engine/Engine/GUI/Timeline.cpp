//-----------------------------------------------------------------------------------
// file:    Timeline.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file implements the Timeline class, handling the initialization and
//          rendering of the timeline UI. It manages the grid structure, playback
//          controls, playhead movement, dynamic layer resizing, and processing of
//          drag-and-drop events to place image clips onto the grid.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "Timeline.h"
#include "imgui.h"
#include "../TypeDef.h"

Timeline::Timeline()
  : currentFrame(0),
  numFrames(100),           //total frames
  zoom(1.0f),               //zooms the x axis of the grid
  cellSize(ImVec2(100, 90)), //
  is_playing(false),         // Start in a paused state
  playbackSpeed(30.0f),     // Default speed of 30 frames per second
  lastFrameTime(0.0)        // Initialize time to zero
{
  // Start with 2 layers
  for (s32 i = 0; i < 2; ++i)
    layers.push_back(i);
}


/*=================================================================================
    Function Brief:
    Handles the entire rendering and interaction logic for the timeline window.
    This includes drawing the grid, handling zoom/playback controls, updating the
    playhead position, processing drag-and-drop events from the SpriteDropper,
    and drawing the placed TimelineClip objects.

    Where to call it:
    Must be called once per frame inside the game state's main Draw() function
    (e.g., SpriteState::Draw()).
    ==================================================================================*/

    // --Timeline--
void Timeline::draw() {
  if (ImGui::Begin("Timeline"))
  {
    ImVec2 timelineSize(cellSize.x * numFrames, cellSize.y * (int)layers.size());
    ImVec2 incrementchange(5, 5);

    // --- Control buttons ---
    if (ImGui::Button("Add")) { //adds layer
      layers.push_back((int)layers.size());
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove") && !layers.empty() && layers.size() != 1) { //removes layer
      layers.pop_back();
    }



    // zoom button and slider
    ImGui::SameLine();
    if (ImGui::Button("-")) { //zooms out timeline
      zoom -= 0.5f;
      // Clamp the min zoom level
      if (zoom < 0.2f) zoom = 0.2f;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::SliderFloat(" ", &zoom, 0.05f, 3.0f, "%.1fx"); //zoom slider

    ImGui::SameLine();
    if (ImGui::Button("+")) { //zooms in timeline
      zoom += 0.5f;
      // You can clamp the max zoom level if needed
      if (zoom > 5.0f) zoom = 5.0f;
    }

    // Add play/pause button
    ImGui::SameLine();
    if (ImGui::Button(is_playing ? "Pause" : "Play")) {
      is_playing = !is_playing;
      // When playback starts, reset the last frame time
      if (is_playing) {
        lastFrameTime = ImGui::GetTime();
      }
    }



    // --- Timeline region ---
    ImGui::BeginChild("TimelineRegion", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();


    ImVec2 currentCellSize = ImVec2(cellSize.x * zoom, cellSize.y);

    //Draw grid & cells
    for (s32 layer = 0; layer < (int)layers.size(); ++layer) {
      for (s32 frame = 0; frame < numFrames; ++frame) {
        ImVec2 cellMin = ImVec2(p.x + frame * currentCellSize.x, p.y + layer * currentCellSize.y);
        ImVec2 cellMax = ImVec2(cellMin.x + currentCellSize.x, cellMin.y + currentCellSize.y);

        // Draw background rect
        draw_list->AddRect(cellMin, cellMax, IM_COL32(100, 100, 100, 255));

        // Make cell clickable
        ImGui::SetCursorScreenPos(cellMin);
        if (ImGui::InvisibleButton(
          (std::string("cell_") + std::to_string(layer) + "_" + std::to_string(frame)).c_str(),
          currentCellSize))
        {
          currentFrame = frame;
          is_playing = false; // Pause playback on manual selection
        }
      }
    }

    // Draw playhead to place marker
    ImVec2 headMin = ImVec2(p.x + currentFrame * currentCellSize.x, p.y);
    ImVec2 headMax = ImVec2(headMin.x + 2, p.y + (int)layers.size() * currentCellSize.y);
    draw_list->AddRectFilled(headMin, headMax, IM_COL32(255, 0, 0, 255));

    // Update playhead position if playing
    if (is_playing) {
      // Get the current time and calculate the time elapsed since the last frame
      f64 currentTime = ImGui::GetTime();
      f32 deltaTime = (f32)(currentTime - lastFrameTime);

      // Advance the current frame based on the playback speed and delta time
      currentFrame += static_cast<s32>(playbackSpeed * deltaTime);

      // Check if the marker has reached or passed the end of the timeline
      if (currentFrame >= numFrames) {
        // Stop playback by setting isPlaying to false
        is_playing = false;
        currentFrame = numFrames - 1; //clamp to current frame
      }

      // Update the last frame time for the next iteration
      lastFrameTime = currentTime;
    }



    ImGui::EndChild();
  }
  ImGui::End();
}
