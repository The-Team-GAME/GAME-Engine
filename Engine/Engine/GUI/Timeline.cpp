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
#include <cmath>


Timeline::Timeline()
  : currentFrame(0),
    numFrames(100),            // total frames
    zoom(1.0f),                // zooms the x axis of the grid
    cellSize(ImVec2(100, 90)), // well...it explains what it does in the name....
    is_playing(false),         // Start in a paused state
    playbackSpeed(30.0f),      // Default speed of 30 frames per second
    lastFrameTime(0.0),        // Initialize time to zero
    m_frameAccumulator(0.0),   // floating point accumulator
    m_nextClipID(1),           // 
    m_clipBeingResized(0)      //
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
          m_frameAccumulator = (f64)frame;
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
      f64 deltaTime = (f32)(currentTime - lastFrameTime);

      lastFrameTime = currentTime;

      // Advance the playhead position using the float accumulator
      m_frameAccumulator += (playbackSpeed * deltaTime);

      // *** FIX 2: Implement the cycling logic (Loop the playhead) ***
      if (m_frameAccumulator >= numFrames) {
        // Cycle the playhead position back to the start, preserving any overshoot time
        // E.g., if total frames is 100, and currentFramePos is 100.5, this sets it to 0.5.
        m_frameAccumulator = std::fmod(m_frameAccumulator, (f64)numFrames);

        // Alternative simple reset (less precise):
        // m_frameAccumulator = 0.0;

        // is_playing remains 'true', so playback continues (cycles).
      }

      // Update the integer frame counter for use in the rest of your system
      currentFrame = static_cast<s32>(m_frameAccumulator);
    }

    // --- Drag-and-Drop Logic for NEW Clips ---
    // NOTE: "ASSET_TEXTURE_ID" is an example payload type. Use the string your Asset Browser uses.
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE_ID")) {

        // Assuming the payload contains an s32/u32 that is the resource ID for your texture.
        s32 texture_id = *static_cast<s32*>(payload->Data);

        // Get drop position and convert screen position to grid coordinates (frame and layer)
        ImVec2 dropPos = ImGui::GetMousePos();
        ImVec2 relativePos = ImVec2(dropPos.x - p.x, dropPos.y - p.y);

        // Calculate the starting frame and layer
        s32 dropped_frame = (s32)std::floor(relativePos.x / currentCellSize.x);
        s32 dropped_layer = (s32)std::floor(relativePos.y / currentCellSize.y);

        // Clamp to valid ranges
        dropped_frame = std::max(0, std::min(dropped_frame, numFrames - 1));
        dropped_layer = std::max(0, std::min(dropped_layer, (s32)layers.size() - 1));

        // Create and add the new clip
        TimelineClip newClip;
        newClip.id = m_nextClipID++; // Assign unique ID
        newClip.texture_id = texture_id;
        newClip.layer = dropped_layer;
        newClip.start_frame = dropped_frame;
        newClip.duration_frames = 30; // Default duration

        clips.push_back(newClip);
      }
      ImGui::EndDragDropTarget();
    }


    // --- Clip Drawing and Resizing Logic ---
    f32 frameWidth = currentCellSize.x;
    ImVec2 mousePos = ImGui::GetMousePos();

    for (TimelineClip& clip : clips) {
      // 1. Calculate the clip's screen coordinates
      f32 x_start = p.x + clip.start_frame * frameWidth;
      f32 x_end = p.x + (clip.start_frame + clip.duration_frames) * frameWidth;
      f32 y_start = p.y + clip.layer * currentCellSize.y;
      f32 y_end = p.y + (clip.layer + 1) * currentCellSize.y;

      ImVec2 clipMin(x_start, y_start);
      ImVec2 clipMax(x_end, y_end);

      // Skip drawing if clip is entirely off-screen
      if (x_end < p.x || x_start > p.x + numFrames * frameWidth) continue;

      // 2. Draw the clip
      ImU32 clipColor = IM_COL32(0, 150, 255, 150);
      draw_list->AddRectFilled(clipMin, clipMax, clipColor, 3.0f);
      draw_list->AddRect(clipMin, clipMax, IM_COL32(0, 100, 200, 255), 3.0f);

      // Draw clip info 
      // *** FIX for E0349: Manually calculate the new position components ***
      ImVec2 textPos;
      textPos.x = clipMin.x + 5.0f;
      textPos.y = clipMin.y + 5.0f;
      ImGui::SetCursorScreenPos(textPos);

      // This line is now safe
      ImGui::Text("Tex: %d | Start: %d | Dur: %d", clip.texture_id, clip.start_frame, clip.duration_frames);


      // 3. Resizing Logic on the X-axis (Right Edge)
      f32 resizeHandleWidth = 10.0f;
      ImVec2 handleMin(x_end - resizeHandleWidth, y_start);
      ImVec2 handleMax(x_end, y_end);

      bool is_hovering_handle = mousePos.x >= handleMin.x && mousePos.x <= handleMax.x &&
        mousePos.y >= handleMin.y && mousePos.y <= handleMax.y;

      if (is_hovering_handle || m_clipBeingResized == clip.id) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
      }

      // Invisible button to capture click/drag state on the handle
      ImGui::SetCursorScreenPos(handleMin);
      std::string resizeID = "resize_handle_" + std::to_string(clip.id);
      ImGui::InvisibleButton(resizeID.c_str(), ImVec2(resizeHandleWidth, currentCellSize.y));

      if (ImGui::IsItemActive()) {
        m_clipBeingResized = clip.id;
      }

      // 4. Update duration if this clip is actively being resized
      if (m_clipBeingResized == clip.id) {
        if (ImGui::IsMouseDown(0)) {
          f32 mouseDeltaX = ImGui::GetIO().MouseDelta.x;
          s32 frameDelta = (s32)std::round(mouseDeltaX / frameWidth);

          if (frameDelta != 0) {
            s32 newDuration = clip.duration_frames + frameDelta;
            clip.duration_frames = std::max(1, newDuration);
          }

        }
        else {
          m_clipBeingResized = 0;
        }
      }
    }


    ImGui::EndChild();
  }
  ImGui::End();
}


void Timeline::add_clip(s32 texture_id, s32 layer, s32 start_frame, s32 duration_frames) {
  TimelineClip newClip;
  newClip.id = m_nextClipID++; // Assign unique ID
  newClip.texture_id = texture_id;
  newClip.layer = layer;
  newClip.start_frame = start_frame;
  newClip.duration_frames = duration_frames;

  // Ensure the clip is within valid bounds (optional, but good practice)
  newClip.layer = std::max(0, std::min(newClip.layer, (s32)layers.size() - 1));
  newClip.start_frame = std::max(0, std::min(newClip.start_frame, numFrames - 1));
  newClip.duration_frames = std::max(1, newClip.duration_frames);

  clips.push_back(newClip);
}
