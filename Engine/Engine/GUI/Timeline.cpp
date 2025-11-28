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
#include "../Precompiled.h"
#include "../TypeDef.h"
#include <cmath>
#include "../Logger.h"
#include "../ECS/ECS.h"
#include "../../Engine/MonkeyEngine.h"
#include "ResourcePicker.h"

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




void Timeline::add_clip(Engine::ECS::Entity entity, s32 layer, s32 start_frame, s32 duration_frames) {
  TimelineClip newClip;
  newClip.id = m_nextClipID++;
  newClip.entity = entity; // Store the full entity object
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


    ImGui::SameLine();

    // ================================================================
    // Resource Picker Button and Logic
    // ================================================================

    if (ImGui::Button("Add Texture Clip")) {
      is_texture_picker_open = true;
      pending_texture_path.clear();
      ImGui::OpenPopup("TexturePickerPopup");
    }

    // 1. Check for Resource Picker result
    if (is_texture_picker_open) {
      bool selection_made = ResourcePicker::texture_picker_popup(
        "TexturePickerPopup",
        pending_texture_path,
        current_resource_path,
        RESOURCE_GRID_COLUMNS,
        RESOURCE_PREVIEW_SIZE
      );

      if (selection_made) {
        is_texture_picker_open = false;
        if (!pending_texture_path.empty()) {
          // Selection made: NOW OPEN THE CONFIGURATION MODAL
          is_config_modal_open = true;
          // Reset config values to safe defaults/current timeline state
          config_layer = (s32)layers.size() > 0 ? (s32)layers.size() - 1 : 0;
          config_start_frame = currentFrame;
          config_duration = 30;
          config_atlas_x = 1;
          config_atlas_y = 1;

          ImGui::OpenPopup("ClipConfigurationModal");
        }
        else {
          // Path was empty (shouldn't happen with the picker's return logic, but safe to reset)
          pending_texture_path.clear();
        }
      }
    }

    // 2. Draw the Configuration Modal
    draw_clip_config_modal();


    // controls entity visibility and animation frame in the game world.
    for (TimelineClip& clip : clips) {
      bool is_active_in_scene = (currentFrame >= clip.start_frame &&
        currentFrame < (clip.start_frame + clip.duration_frames));

      // We use clip.entity directly, which is the correct ECS::Entity object.
      if (clip.entity.is_valid() && clip.entity.has_component<Components::TextureAtlas>()) {
        auto* atlas = clip.entity.get_component<Components::TextureAtlas>();

        if (is_active_in_scene) {
          // --- Calculate which atlas frame to show ---
          s32 frames_in_clip = currentFrame - clip.start_frame;
          s32 total_clip_duration = clip.duration_frames;
          s32 num_atlas_frames = (s32)atlas->subTextures.size();

          s32 frame_index = 0; // Default to the first frame

          if (num_atlas_frames > 0) {
            // Spreads N atlas frames evenly over the clip's duration
            frame_index = (s32)std::floor((f32)frames_in_clip / total_clip_duration * num_atlas_frames);
            frame_index = std::min(frame_index, num_atlas_frames - 1);
          }

          // Enables rendering by setting a valid frame index
          atlas->drawIndex = frame_index;
        }
        else {
          // Disables rendering by setting the sentinel value
          atlas->drawIndex = -1;
        }
      }
    }



    // --- Timeline region ---
    ImGui::BeginChild("TimelineRegion", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);
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
          auto entity = activeClip->entity; // Access the stored ECS::Entity

          if (entity.is_valid() && entity.has_component<Components::Texture>() && entity.has_component<Components::TextureAtlas>())
          {
            auto texture_ptr = entity.get_component<Components::Texture>()->texture_handle;
            auto* atlas = entity.get_component<Components::TextureAtlas>();


            s32 num_atlas_frames = (s32)atlas->subTextures.size();

            if (texture_ptr && num_atlas_frames > 0) {
              ImTextureID tex_id = (ImTextureID)(uintptr_t)texture_ptr->id;

              // Calculate which frame of the atlas should be shown in this timeline cell
              s32 frames_in_clip = frame - activeClip->start_frame;
              s32 total_clip_duration = activeClip->duration_frames;

              // Spreads the N atlas frames evenly over the clip's duration
              size_t frame_index = (size_t)std::floor((f32)frames_in_clip / total_clip_duration * num_atlas_frames);
              frame_index = std::min(frame_index, (size_t)num_atlas_frames - 1);

              // texture coordinates for the selected frame
              const auto& tex_coords = atlas->subTextures[frame_index].tex_coords;

              // Top-Left (Index 3: (u0, v0))
              ImVec2 uv0(tex_coords[3].x, tex_coords[3].y);
              // Bottom-Right (Index 1: (u1, v1))
              ImVec2 uv1(tex_coords[1].x, tex_coords[1].y);

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


    //// ----------------------------------------------------------------------
    //// --- Drag-and-Drop Logic ---
    //// ----------------------------------------------------------------------

    //if (ImGui::BeginDragDropTarget()) {
    //  if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE_ID")) {
    //    s32 texture_id = *static_cast<s32*>(payload->Data);

    //    //Engine::Graphics::Texture texture_object = Timeline::set_selected_entity_id(texture_id);
    //    //
    //    //// Create the entity, which includes the 1x1 TextureAtlas
    //    //Engine::Graphics::Texture texture_object = Engine::Graphics::TextureData();

    //    //s32 entity_id = new_entity.get_id();

    //    ImVec2 dropPos = ImGui::GetMousePos();
    //    ImVec2 relativePos = ImVec2(dropPos.x - p.x, dropPos.y - p.y);
    //    s32 dropped_frame = (s32)std::floor(relativePos.x / currentCellSize.x);
    //    s32 dropped_layer = (s32)std::floor(relativePos.y / currentCellSize.y);

    //    dropped_frame = std::max(0, std::min(dropped_frame, numFrames - 1));
    //    dropped_layer = std::max(0, std::min(dropped_layer, (s32)layers.size() - 1));

    //    add_clip(texture_id, dropped_layer, dropped_frame, 30);
    //  }
    //  ImGui::EndDragDropTarget();
    //}



    // ----------------------------------------------------------------------
    // --- Clip Manipulation Logic ---
    // ----------------------------------------------------------------------

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

      ImGui::Text("| Dur: %d |", clip.duration_frames);




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
        m_dragStartFrame = clip.start_frame;
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

      if (clip.id == m_activeClipID && ImGui::IsMouseDown(0)) {

        ImVec2 dragDelta = ImGui::GetMouseDragDelta(0);
        f32 frame_width = frameWidth; // Use existing local variable

        // --- Move Frame ---
        if (m_manipulationMode == 1) {
          Log::info("move");
          // Calculate total frames moved since the drag started
          s32 frames_moved = (s32)std::round(dragDelta.x / frame_width);
          s32 new_start_frame = m_dragStartFrame + frames_moved;

          // Clamp and update the position
          s32 clamped_start_frame = std::max(0, new_start_frame);
          clamped_start_frame = std::min(clamped_start_frame, numFrames - clip.duration_frames);

          // Crucial Snap Logic: Only update clip and reset delta if a full frame boundary was crossed
          if (clamped_start_frame != clip.start_frame) {
            clip.start_frame = clamped_start_frame;
            // Reset delta to zero to prevent accumulation and jumping
            ImGui::ResetMouseDragDelta(0);
            // Update the drag anchor to the new snapped position
            m_dragStartFrame = clip.start_frame;
          }
          ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        // --- Resize Right ---
        else if (m_manipulationMode == 2) {
          s32 frames_changed = (s32)std::round(dragDelta.x / frame_width);
          s32 new_duration = m_dragStartDuration + frames_changed;

          // Clamp duration to minimum 1 frame
          clip.duration_frames = std::max(1, new_duration);
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        // --- Resize Left ---
        else if (m_manipulationMode == 3) {
          s32 frames_changed = (s32)std::round(dragDelta.x / frame_width);
          s32 old_end_frame = m_dragStartFrame + m_dragStartDuration;
          s32 new_start_frame = m_dragStartFrame + frames_changed;
          s32 new_duration = old_end_frame - new_start_frame;

          if (new_duration >= 1 && new_start_frame >= 0) {
            clip.start_frame = new_start_frame;
            clip.duration_frames = new_duration;
          }
          else if (new_start_frame < 0) { // Clamp start frame to 0
            clip.start_frame = 0;
            clip.duration_frames = old_end_frame;
          }
          else if (new_duration < 1) { // Clamp duration to 1 frame minimum
            clip.start_frame = old_end_frame - 1;
            clip.duration_frames = 1;
          }
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
      }
      // ---  Hover Logic ---
      else {
        // Original Hover Checks for Cursor
        if ((mousePos.x >= handleRMin.x && mousePos.x <= handleRMax.x && mousePos.y >= handleRMin.y && mousePos.y <= handleRMax.y) ||
          (mousePos.x >= handleLMin.x && mousePos.x <= handleLMax.x && mousePos.y >= handleLMin.y && mousePos.y <= handleLMax.y)) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
        else if (mousePos.x >= dragMin.x && mousePos.x <= dragMax.x && mousePos.y >= dragMin.y && mousePos.y <= dragMax.y) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
      }
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


    // Check for end of manipulation
    if (m_activeClipID != 0 && !ImGui::IsMouseDown(0)) {
      m_activeClipID = 0;
      m_manipulationMode = 0;
    }

    ImGui::EndChild();
  }
  ImGui::End();
}

namespace TimelineUtils {
  Engine::ECS::Entity create_timeline_entity(const std::string& texture_path, s32 atlas_x_count, s32 atlas_y_count)
  {
    // Load the texture handle
    Engine::Graphics::Texture texture_value = Engine::Resources::load_texture(texture_path, false);

    // Check validity
    if (!texture_value.operator->()) {
      return Engine::ECS::create_entity();
    }

    Engine::ECS::Entity new_entity = Engine::ECS::create_entity();

    // Add required components (Using placeholder values)
    new_entity.add_component<Components::Transform2D>(400.0f, 400.0f, 0.0f, 100.0f, 100.0f);
    new_entity.add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);
    new_entity.add_component<Components::Texture>(texture_value);

    // Initialize Texture Atlas with provided counts
    s32 total_sprites = atlas_x_count * atlas_y_count;
    f32 texture_width = (f32)texture_value->width;
    f32 texture_height = (f32)texture_value->height;

    new_entity.add_component<Components::TextureAtlas>(
      (int)(texture_width / atlas_x_count),
      (int)(texture_height / atlas_y_count),
      (u32)atlas_y_count,
      (u32)atlas_x_count,
      (u32)total_sprites
    );

    Components::TextureAtlas* atlas = new_entity.get_component<Components::TextureAtlas>();
    if (atlas) {
      atlas->drawIndex = -1;
    }

    return new_entity;
  }
}

void Timeline::draw_clip_config_modal() {
  if (is_config_modal_open) {
    // Set fixed size for the modal
    ImGui::SetNextWindowSize(ImVec2(350, 300), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal("ClipConfigurationModal", &is_config_modal_open, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Configure New Timeline Clip");
      ImGui::Separator();

      // --- Timeline Parameters ---
      ImGui::Text("Timeline Placement");
      ImGui::InputInt("Layer", &config_layer);
      config_layer = std::max(0, std::min(config_layer, (s32)layers.size() > 0 ? (s32)layers.size() - 1 : 0));

      ImGui::InputInt("Start Frame", &config_start_frame);
      config_start_frame = std::max(0, std::min(config_start_frame, numFrames - 1));

      ImGui::InputInt("Duration (Frames)", &config_duration);
      config_duration = std::max(1, config_duration);

      ImGui::Separator();

      // --- Animation Atlas Parameters ---
      ImGui::Text("Texture Atlas");
      ImGui::TextDisabled("Path: %s", pending_texture_path.c_str());

      ImGui::InputInt("Sprites per Row (X)", &config_atlas_x);
      config_atlas_x = std::max(1, config_atlas_x); // Min 1x1 atlas

      ImGui::InputInt("Sprites per Column (Y)", &config_atlas_y);
      config_atlas_y = std::max(1, config_atlas_y); // Min 1x1 atlas

      ImGui::Separator();

      // --- Confirmation ---
      if (ImGui::Button("Create Clip", ImVec2(100, 0))) {
        // Create the ECS Entity using the standalone utility function
        Engine::ECS::Entity new_entity = TimelineUtils::create_timeline_entity(
          pending_texture_path,
          config_atlas_x,
          config_atlas_y
        );

        if (new_entity.is_valid()) { // Check if creation was successful
          // Add clip to the timeline data structure, passing the entity object
          add_clip(new_entity, config_layer, config_start_frame, config_duration);
        }

        // Clear all temporary states and close popup
        pending_texture_path.clear();
        is_config_modal_open = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(100, 0))) {
        // Clear temporary states and close popup
        pending_texture_path.clear();
        is_config_modal_open = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }
}



