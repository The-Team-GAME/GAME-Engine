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
#include <iostream>
#include "../ECS/ECS.h"

const f32 LEFT_RESIZE_HANDLE_WIDTH = 10.0f;
const f32 RIGHT_RESIZE_HANDLE_WIDTH = 10.0f;

Timeline::Timeline()
  : currentFrame(0),
    numFrames(100),            // total frames
    zoom(1.0f),                // zooms the x axis of the grid
    cellSize(ImVec2(100, 90)), // well...it explains what it does in the name....
    is_playing(false),         // Start in a paused state
    playbackSpeed(30.0f),      // Default speed of 30 frames per second
    lastFrameTime(0.0),        // Initialize time to zero
    m_frameAccumulator(0.0),   // floating point accumulator
    m_nextClipID(1),           // Unique ID generator
    m_activeClipID(0),
    m_manipulationMode(0),
    m_dragStartFrame(0),
    m_dragStartDuration(0),
    m_selectedEntityID(0)
{
  // Start with 2 layers
  for (s32 i = 0; i < 2; ++i)
    layers.push_back(i);
}




void Timeline::add_clip(s32 texture_id, s32 layer, s32 start_frame, s32 duration_frames) {
  TimelineClip newClip;
  newClip.id = m_nextClipID++; // Assign unique ID
  newClip.texture_id = texture_id;
  newClip.layer = layer;
  newClip.start_frame = start_frame;
  newClip.duration_frames = duration_frames;
  newClip.layer = std::max(0, newClip.layer);
  newClip.start_frame = std::max(0, newClip.start_frame);
  newClip.duration_frames = std::max(1, newClip.duration_frames);

  clips.push_back(newClip);
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

    //layer adding logic
    if (ImGui::Button("Add")) { //adds layer
      layers.push_back((int)layers.size());
    }
    ImGui::SameLine();

    ImGui::SameLine();
    if (ImGui::Button("Remove") && !layers.empty() && layers.size() != 1) { //removes layer

      s32 layer_to_remove = (s32)layers.size() - 1; // Index of the layer about to be removed
      bool clips_exist = false;

      // Check for existing clips on the layer to remove
      for (const auto& clip : clips) {
        if (clip.layer == layer_to_remove) {
          clips_exist = true;
          break;
        }
      }

      if (clips_exist) {
        // --- Show Warning Modal ---
        ImGui::OpenPopup("Confirm Layer Deletion");
      }
      else {
        // No clips, safe to remove immediately
        layers.pop_back();

        // After removing the layer, adjust all remaining clip layers that were above it
        // (Not needed if you only pop_back(), but good practice if removing a middle layer)
      }
    }

    //  Layer removing logic
    if (ImGui::BeginPopupModal("Confirm Layer Deletion", NULL, ImGuiWindowFlags_Modal | ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("WARNING: Deleting this layer will remove all %d clips on it.",
        std::count_if(clips.begin(), clips.end(), [&](const TimelineClip& c) {
          return c.layer == (s32)layers.size() - 1;
          }));
      ImGui::Text("Are you sure you want to proceed?");

      if (ImGui::Button("Yes, Delete All") && !layers.empty()) {
        s32 layer_to_remove = (s32)layers.size() - 1;

        // 1. Remove all clips associated with this layer
        clips.erase(std::remove_if(clips.begin(), clips.end(),
          [&](const TimelineClip& c) {
            return c.layer == layer_to_remove;
          }), clips.end());

        // 2. Remove the layer itself
        layers.pop_back();

        // 3. Close the modal
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
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




    //gets the entity
    if (m_selectedEntityID != 0) {
      auto entity = Engine::ECS::get_entity(m_selectedEntityID);
    }



    // --- Timeline region ---
    ImGui::BeginChild("TimelineRegion", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos(); // Top-left corner of the scrollable region


    ImVec2 currentCellSize = ImVec2(cellSize.x * zoom, cellSize.y);
    f32 frameWidth = currentCellSize.x;
    ImVec2 mousePos = ImGui::GetMousePos();

    //Draw grid & cells (Grid logic remains here, updated to use m_frameAccumulator for clicking)
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
          m_frameAccumulator = (f64)frame; // Update accumulator too
          is_playing = false; // Pause playback on manual selection
        }
      }
    }

    // --- Drag-and-Drop Logic for NEW Clips ---
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE_ID")) {
        s32 texture_id = *static_cast<s32*>(payload->Data);
        ImVec2 dropPos = ImGui::GetMousePos();
        ImVec2 relativePos = ImVec2(dropPos.x - p.x, dropPos.y - p.y);
        s32 dropped_frame = (s32)std::floor(relativePos.x / currentCellSize.x);
        s32 dropped_layer = (s32)std::floor(relativePos.y / currentCellSize.y);

        dropped_frame = std::max(0, std::min(dropped_frame, numFrames - 1));
        dropped_layer = std::max(0, std::min(dropped_layer, (s32)layers.size() - 1));

        add_clip(texture_id, dropped_layer, dropped_frame, 30);
      }
      ImGui::EndDragDropTarget();
    }


    // --- Clip Drawing and Manipulation Logic ---

    // 1. Check for end of manipulation
    if (m_activeClipID != 0 && !ImGui::IsMouseDown(0)) {
      m_activeClipID = 0;
      m_manipulationMode = 0;
      std::cout << "oh...your pressingggg on me<3<3";
    }

    for (TimelineClip& clip : clips) {
      // 2. Calculate the clip's screen coordinates
      f32 x_start = p.x + clip.start_frame * frameWidth;
      f32 x_end = p.x + (clip.start_frame + clip.duration_frames) * frameWidth;
      f32 y_start = p.y + clip.layer * currentCellSize.y;
      f32 y_end = p.y + (clip.layer + 1) * currentCellSize.y;

      ImVec2 clipMin(x_start, y_start);
      ImVec2 clipMax(x_end, y_end);

      if (x_end < p.x || x_start > p.x + numFrames * frameWidth) continue;

      // 3. Draw the clip background
      ImU32 clipColor = IM_COL32(0, 150, 255, 150);
      draw_list->AddRectFilled(clipMin, clipMax, clipColor, 3.0f);
      draw_list->AddRect(clipMin, clipMax, IM_COL32(0, 100, 200, 255), 3.0f);


      // *** FIX 1: Draw the small Image iteration (Thumbnail) ***
      ImTextureID tex_id = (ImTextureID)(intptr_t)clip.texture_id;
      float inset = 2.0f;
      float thumbSize = currentCellSize.y - inset * 2.0f; // Use height as thumb size


      draw_list->AddImage(
        tex_id,
        ImVec2(clipMin.x + inset, clipMin.y + inset),
        ImVec2(clipMin.x + inset + thumbSize, clipMin.y + inset + thumbSize), // Fixed width based on height
        ImVec2(0, 0), ImVec2(1, 1),
        IM_COL32_WHITE
      );

      ImVec2 textPos;
      // Manually add components to offset past the thumbnail
      textPos.x = clipMin.x + thumbSize + inset * 2.0f;
      textPos.y = clipMin.y + inset;
      ImGui::SetCursorScreenPos(textPos);

      ImGui::Text("Tex: %d | Start: %d | Dur: %d", clip.texture_id, clip.start_frame, clip.duration_frames);


      // --- Manipulation Handlers ---

      // A. Resize Right Handle (X-Axis Scaling)
      ImVec2 handleRMin(x_end - RIGHT_RESIZE_HANDLE_WIDTH, y_start);
      ImVec2 handleRMax(x_end, y_end);

      ImGui::SetCursorScreenPos(handleRMin);
      std::string resizeRID = "resize_R_handle_" + std::to_string(clip.id);
      ImGui::InvisibleButton(resizeRID.c_str(), ImVec2(RIGHT_RESIZE_HANDLE_WIDTH, currentCellSize.y));

      if (ImGui::IsItemActive() && m_activeClipID == 0) {
        m_activeClipID = clip.id;
        m_manipulationMode = 2; // Resize Right
        m_dragStartDuration = clip.duration_frames;
      }

      // B. Resize Left Handle (Two-sided scaling)
      ImVec2 handleLMin(x_start, y_start);
      ImVec2 handleLMax(x_start + LEFT_RESIZE_HANDLE_WIDTH, y_end);

      ImGui::SetCursorScreenPos(handleLMin);
      std::string resizeLID = "resize_L_handle_" + std::to_string(clip.id);
      ImGui::InvisibleButton(resizeLID.c_str(), ImVec2(LEFT_RESIZE_HANDLE_WIDTH, currentCellSize.y));

      if (ImGui::IsItemActive() && m_activeClipID == 0) {
        m_activeClipID = clip.id;
        m_manipulationMode = 3; // Resize Left
        m_dragStartFrame = clip.start_frame;
        m_dragStartDuration = clip.duration_frames;
      }

      // C. Drag/Move Handle (Pull/Move the image)
      ImVec2 dragMin(x_start + LEFT_RESIZE_HANDLE_WIDTH, y_start);
      ImVec2 dragMax(x_end - RIGHT_RESIZE_HANDLE_WIDTH, y_end);

      ImGui::SetCursorScreenPos(dragMin);
      std::string dragID = "drag_clip_" + std::to_string(clip.id);
      ImGui::InvisibleButton(dragID.c_str(), ImVec2(dragMax.x - dragMin.x, currentCellSize.y));

      if (ImGui::IsItemActive() && m_activeClipID == 0) {
        m_activeClipID = clip.id;
        m_manipulationMode = 1; // Move
        m_dragStartFrame = clip.start_frame;
      }

      // Apply visual cursor change based on hovering
      if (m_activeClipID == clip.id) {
        if (m_manipulationMode == 1) { // Move
          ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        else if (m_manipulationMode == 2 || m_manipulationMode == 3) { // Resize
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
      }
      else if ((mousePos.x >= handleRMin.x && mousePos.x <= handleRMax.x && mousePos.y >= handleRMin.y && mousePos.y <= handleRMax.y) ||
        (mousePos.x >= handleLMin.x && mousePos.x <= handleLMax.x && mousePos.y >= handleLMin.y && mousePos.y <= handleLMax.y)) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
      }
      else if (mousePos.x >= dragMin.x && mousePos.x <= dragMax.x && mousePos.y >= dragMin.y && mousePos.y <= dragMax.y) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
      }
    }


    // --- 5. Global Manipulation Update Logic (applied to the active clip) ---
    if (m_activeClipID != 0 && ImGui::IsMouseDown(0)) {
      TimelineClip* activeClip = nullptr;
      for (TimelineClip& clip : clips) {
        if (clip.id == m_activeClipID) {
          activeClip = &clip;
          break;
        }
      }

      if (activeClip) {
        std::cout << "hello!";
        f32 dragDeltaX = ImGui::GetMouseDragDelta(0).x;
        s32 frameDelta = (s32)std::round(dragDeltaX / frameWidth);

        // Calculates current frame under mouse for RESIZING
        s32 currentDragFrame = (s32)std::round((mousePos.x - p.x) / frameWidth);

        if (m_manipulationMode == 1) { // Move
          s32 newStartFrame = m_dragStartFrame + frameDelta;
          activeClip->start_frame = std::max(0, newStartFrame);
        }
        else if (m_manipulationMode == 2) { // Resize Right
          s32 newEndFrame = currentDragFrame;
          s32 newDuration = newEndFrame - activeClip->start_frame;
          activeClip->duration_frames = std::max(1, newDuration);
          std::cout << "righting";
        }
        else if (m_manipulationMode == 3) { // Resize Left
          s32 newStartFrame = currentDragFrame;
          s32 oldEndFrame = m_dragStartFrame + m_dragStartDuration;
          s32 newDuration = oldEndFrame - newStartFrame;

          if (newDuration >= 1 && newStartFrame >= 0) {
            activeClip->start_frame = newStartFrame;
            activeClip->duration_frames = newDuration;
          }
          else if (newStartFrame < 0) {
            activeClip->start_frame = 0;
            activeClip->duration_frames = oldEndFrame;
          }
        }
      }
    }


    // Draw playhead to place marker (updated to use m_frameAccumulator for smooth drawing)
    ImVec2 headMin = ImVec2(p.x + (f32)m_frameAccumulator * currentCellSize.x, p.y);
    ImVec2 headMax = ImVec2(headMin.x + 2, p.y + (int)layers.size() * currentCellSize.y);
    draw_list->AddRectFilled(headMin, headMax, IM_COL32(255, 0, 0, 255));

    // Update playhead position if playing (updated for smooth movement/looping)
    if (is_playing) {
      f64 currentTime = ImGui::GetTime();
      f64 deltaTime = (currentTime - lastFrameTime); // Use f64 for precision

      lastFrameTime = currentTime;

      m_frameAccumulator += (playbackSpeed * deltaTime);

      if (m_frameAccumulator >= numFrames) {
        m_frameAccumulator = std::fmod(m_frameAccumulator, (f64)numFrames); // Loop
      }

      currentFrame = static_cast<s32>(m_frameAccumulator);
    }


    ImGui::EndChild();
  }
  ImGui::End();
};

