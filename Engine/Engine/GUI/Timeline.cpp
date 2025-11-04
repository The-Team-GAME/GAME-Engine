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
// Copyright � 2025 DigiPen Institute of Technology Singapore.\ All rights reserved.
//-----------------------------------------------------------------------------------

#include "Timeline.h"
#include "imgui.h"
#include "../TypeDef.h"
#include <cmath>
#include <iostream>
#include "../ECS/ECS.h"
#include "../../Engine/MonkeyEngine.h"

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
  // Start with 3 layers
  for (s32 i = 0; i < 3; ++i)
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

        // Remove all clips associated with this layer
        clips.erase(std::remove_if(clips.begin(), clips.end(),
          [&](const TimelineClip& c) {
            return c.layer == layer_to_remove;
          }), clips.end());

        // Remove the layer itself
        layers.pop_back();

        // Close the modal
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
      // clamps the min zoom level
      if (zoom < 0.2f) zoom = 0.2f;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::SliderFloat(" ", &zoom, 0.05f, 3.0f, "%.1fx"); //zoom slider

    ImGui::SameLine();
    if (ImGui::Button("+")) { //zooms in timeline
      zoom += 0.5f;
			// clamps the max zoom level
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

    // ----------------------------------------------------------------------
    // --- Grid Drawing and Animated Cell Visualization ---
    // ----------------------------------------------------------------------

    for (s32 layer = 0; layer < (int)layers.size(); ++layer) {
      for (s32 frame = 0; frame < numFrames; ++frame) {
        ImVec2 cellMin = ImVec2(p.x + frame * currentCellSize.x, p.y + layer * currentCellSize.y);
        ImVec2 cellMax = ImVec2(cellMin.x + currentCellSize.x, cellMin.y + currentCellSize.y);

        // --- Check for active clip in this cell ---
        TimelineClip* activeClip = get_clip_at_frame(layer, frame);

        // Default grid background
        draw_list->AddRect(cellMin, cellMax, IM_COL32(100, 100, 100, 255));

        if (activeClip) {
          // --- Clip Found: Draw Animated Frame ---
          auto entity = Engine::ECS::get_entity(activeClip->texture_id);

          if (entity.is_valid() && entity.has_component<Components::Texture>() && entity.has_component<Components::TextureAtlas>())
          {
            auto texture_ptr = entity.get_component<Components::Texture>()->texture_handle;
            auto* atlas = entity.get_component<Components::TextureAtlas>();

            s32 num_atlas_frames = (s32)atlas->subTextures.size();

            if (texture_ptr && num_atlas_frames > 0) {
              ImTextureID tex_id = (ImTextureID)(intptr_t)texture_ptr->id;

              // Calculate which frame of the atlas should be shown in this timeline cell
              s32 frames_in_clip = frame - activeClip->start_frame;
              s32 total_clip_duration = activeClip->duration_frames;

              // Spreads the N atlas frames evenly over the clip's duration
              size_t frame_index = (size_t)std::floor((f32)frames_in_clip / total_clip_duration * num_atlas_frames);
              frame_index = std::min(frame_index, (size_t)num_atlas_frames - 1);

              // texture coordinates for the selected frame
              ImVec2 uv0(atlas->subTextures[frame_index].tex_coords[3].x, atlas->subTextures[frame_index].tex_coords[3].y); // Top-Left (Index 3 in your constructor)
              ImVec2 uv1(atlas->subTextures[frame_index].tex_coords[1].x, atlas->subTextures[frame_index].tex_coords[1].y); // Bottom-Right (Index 1 in your constructor)

              // Draw the individual frame image onto the grid cell
              draw_list->AddImage(
                tex_id,
                cellMin, cellMax, // Draw into the whole cell area
                uv0, uv1,
                IM_COL32_WHITE
              );

              // Draw a strong border around the cell
              draw_list->AddRect(cellMin, cellMax, IM_COL32(0, 150, 255, 255));
            }
            else {
              // Draw a placeholder for clips that don't have an atlas (or texture issue)
              draw_list->AddRectFilled(cellMin, cellMax, IM_COL32(0, 150, 255, 150));
              draw_list->AddRect(cellMin, cellMax, IM_COL32(0, 100, 200, 255));
            }
          }
          else {
            // Clip not found, default grid color already drawn
          }
        }
          // Make cell clickable
          ImVec2 cell_relative_pos = ImVec2(
            (frame * currentCellSize.x),
            (layer * currentCellSize.y)
          );

          // Place the ImGui cursor at the relative position.
          ImGui::SetCursorPos(cell_relative_pos);
          if (ImGui::InvisibleButton((std::string("cell_") + std::to_string(layer) + "_" + std::to_string(frame)).c_str(), currentCellSize))
          {
            currentFrame = frame;
            m_frameAccumulator = (f64)frame;
            is_playing = false;
          }
        }
      }


      // ----------------------------------------------------------------------
      // --- Drag-and-Drop Logic for NEW Clips ---
      // ----------------------------------------------------------------------

      if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE_ID")) {
          s32 texture_id = *static_cast<s32*>(payload->Data);

          //Engine::Graphics::Texture texture_object = Timeline::set_selected_entity_id(texture_id);
          //
          //// Create the entity, which includes the 1x1 TextureAtlas
          //Engine::Graphics::Texture texture_object = Engine::Graphics::TextureData();

          //s32 entity_id = new_entity.get_id();

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


      // ----------------------------------------------------------------------
      // --- Clip Manipulation Logic ---
      // ----------------------------------------------------------------------

      // Check for end of manipulation
      if (m_activeClipID != 0 && !ImGui::IsMouseDown(0)) {
        m_activeClipID = 0;
        m_manipulationMode = 0;
      }

      for (TimelineClip& clip : clips) {
        // Calculate the clip's screen coordinates
        f32 x_start = p.x + clip.start_frame * frameWidth;
        f32 x_end = p.x + (clip.start_frame + clip.duration_frames) * frameWidth;
        f32 y_start = p.y + clip.layer * currentCellSize.y;
        f32 y_end = p.y + (clip.layer + 1) * currentCellSize.y;

        ImVec2 clipMin(x_start, y_start);
        ImVec2 clipMax(x_end, y_end);

        if (x_end < p.x || x_start > p.x + numFrames * frameWidth) continue;

        // --- Draw Clip Text/Handles OVER the animated cells ---

        // Draw a transparent overlay so manipulation handles are easier to see
        draw_list->AddRectFilled(clipMin, clipMax, IM_COL32(255, 255, 255, 30), 3.0f); // Slight translucent overlay

        // Draw Clip Text
        ImVec2 textPos;
        textPos.x = clipMin.x + 5.0f; // Small padding
        textPos.y = clipMin.y + 5.0f;
        ImGui::SetCursorScreenPos(textPos);

        ImGui::Text("Entity: %d | Dur: %d", clip.texture_id, clip.duration_frames);


        // --- Manipulation Handlers ---

        // Resize Right Handle (X-Axis Scaling)
        ImVec2 handleRMin(x_end - RIGHT_RESIZE_HANDLE_WIDTH, y_start);
        ImVec2 handleRMax(x_end, y_end);

        ImGui::SetCursorScreenPos(handleRMin);
        std::string resizeRID = "resize_R_handle_" + std::to_string(clip.id);
        ImGui::InvisibleButton(resizeRID.c_str(), ImVec2(RIGHT_RESIZE_HANDLE_WIDTH, currentCellSize.y));

        if (ImGui::IsItemActive() && m_activeClipID == 0) {
          m_activeClipID = clip.id;
          m_manipulationMode = 2; // Resize Right
          m_dragStartDuration = clip.duration_frames;
          ImGui::ResetMouseDragDelta(0);
        }

        // Resize Left Handle (Two-sided scaling)
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
          ImGui::ResetMouseDragDelta(0);
        }

        // Drag/Move Handle (Pull/Move the image)
        ImVec2 dragMin(x_start + LEFT_RESIZE_HANDLE_WIDTH, y_start);
        ImVec2 dragMax(x_end - RIGHT_RESIZE_HANDLE_WIDTH, y_end);

        ImGui::SetCursorScreenPos(dragMin);
        std::string dragID = "drag_clip_" + std::to_string(clip.id);
        ImGui::InvisibleButton(dragID.c_str(), ImVec2(dragMax.x - dragMin.x, currentCellSize.y));

        if (ImGui::IsItemActive() && m_activeClipID == 0) { //clicked on clip to drag
          m_activeClipID = clip.id;
          m_manipulationMode = 1; // Move
          m_dragStartFrame = clip.start_frame;
          ImGui::ResetMouseDragDelta(0);
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


      // --- Global Manipulation Update Logic ---
      if (m_activeClipID != 0 && ImGui::IsMouseDown(0)) {
        TimelineClip* activeClip = nullptr;
        for (TimelineClip& clip : clips) {
          if (clip.id == m_activeClipID) {
            activeClip = &clip;
            break;
          }
        }

        if (activeClip) {
          // Get total drag delta and map to frame count
          ImVec2 dragDelta = ImGui::GetMouseDragDelta(0);

          f32 frameFloat = (mousePos.x - p.x) / frameWidth;
          s32 snappedDragFrame = (s32)std::round(frameFloat);

          // Ensure frame index is clamped within bounds
          snappedDragFrame = std::max(0, std::min(snappedDragFrame, numFrames));


          if (m_manipulationMode == 1) { // Move

            // Calculate frame delta from drag distance
            s32 frameDelta = (s32)std::round(dragDelta.x / frameWidth);

            s32 newStartFrame = m_dragStartFrame + frameDelta;

            activeClip->start_frame = std::max(0, newStartFrame);

            ImGui::ResetMouseDragDelta(0);
          }
          else if (m_manipulationMode == 2) { // Resize Right
            // Use the snapped mouse position to define the new end frame
            s32 newEndFrame = snappedDragFrame;

            // The new duration is the difference between the new snapped end frame and the start frame
            s32 newDuration = newEndFrame - activeClip->start_frame;

            // Clamp duration to a minimum of 1 frame
            activeClip->duration_frames = std::max(1, newDuration);
          }
          else if (m_manipulationMode == 3) { // Resize Left
            s32 oldEndFrame = m_dragStartFrame + m_dragStartDuration; // End frame is fixed during left-resize

            // Use the snapped mouse position to define the new start frame
            s32 newStartFrame = snappedDragFrame;
            s32 newDuration = oldEndFrame - newStartFrame;

            if (newDuration >= 1 && newStartFrame >= 0) {
              activeClip->start_frame = newStartFrame;
              activeClip->duration_frames = newDuration;
            }
            else if (newStartFrame < 0) { // Clamp start frame to 0
              activeClip->start_frame = 0;
              activeClip->duration_frames = oldEndFrame;
            }
            else if (newDuration < 1) { // Clamp duration to 1 frame minimum
              activeClip->start_frame = oldEndFrame - 1;
              activeClip->duration_frames = 1;
            }
          }
        }
      }

      // ----------------------------------------------------------------------
      // --- Timeline Extension Handle ---
      // ----------------------------------------------------------------------

     // Define the handle size
      const f32 EXTENSION_HANDLE_WIDTH = 20.0f;
      // Calculate the current total size of the grid content
      f32 timelineContentWidth = currentCellSize.x * numFrames;
      f32 timelineContentHeight = currentCellSize.y * (f32)layers.size();

      // Calculate the handle position (starting right after the last frame)
      // Use the top-left scroll position 'p' as the origin.
      ImVec2 handleMin(p.x + timelineContentWidth, p.y);
      ImVec2 handleMax(handleMin.x + EXTENSION_HANDLE_WIDTH, p.y + timelineContentHeight);

      // Optional: Draw a visual indicator
      draw_list->AddRectFilled(handleMin, handleMax, IM_COL32(255, 100, 100, 50)); // Semi-transparent red box

      // Set cursor position and create the invisible button
      ImGui::SetCursorScreenPos(handleMin);
      std::string extensionID = "timeline_extender";
      ImGui::InvisibleButton(extensionID.c_str(), ImVec2(EXTENSION_HANDLE_WIDTH, timelineContentHeight));


      // Handle Interaction
      if (ImGui::IsItemActive() && ImGui::IsMouseDown(0)) {
        // Get the drag delta since the click started
        ImVec2 dragDelta = ImGui::GetMouseDragDelta(0);

        // Convert the drag distance (in pixels) to a frame count
        // Use the frameWidth (currentCellSize.x * zoom) for snapping
        s32 frameDelta = (s32)std::round(dragDelta.x / frameWidth);

        if (std::abs(frameDelta) > 0) {

          s32 newNumFrames = numFrames + frameDelta;

          // Clamp to a sensible minimum (e.g., your current default of 100 frames)
          numFrames = std::max(100, newNumFrames);

          // Reset the mouse drag delta to zero to ensure frame-snapped movement
          ImGui::ResetMouseDragDelta(0);
        }

        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
      }
      else if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
      }


      // ----------------------------------------------------------------------
      // --- Playhead Logic ---
      // ----------------------------------------------------------------------

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
  }


