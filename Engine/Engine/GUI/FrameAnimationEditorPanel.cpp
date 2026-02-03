//-----------------------------------------------------------------------------------
// file:    FrameAnimationEditorPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of Frame Animation Editor panel for editing state machine
//          animation frames. Provides visual frame selection from texture atlas,
//          custom duration editing per frame, drag-reorder functionality, and
//          live animation preview.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "FrameAnimationEditorPanel.h"
#include "../Precompiled.h"
#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Components/StateMachineComponents.h"
#include "../Time.h"
#include "../Logger.h"
#include "widgets/TextureAtlasWidget.h"
#include <algorithm>

// Global wrapper function for extern declaration in ReflectionFieldWidgets.cpp
namespace ImGuiWidgets {
  void FrameAnimationEditor_Open(Engine::ECS::Entity entity, const std::string& state_name) {
    FrameAnimationEditor::Open(entity, state_name);
  }
}

namespace {
  // Editor state
  bool is_open = false;
  s32 selected_entity_id = -1;
  std::string selected_state_name = "";

  // Frame editing state
  std::vector<int> frame_indices;       // Selected frame indices from atlas
  std::vector<float> frame_durations;   // Duration for each frame
  float default_duration = 0.1f;        // Default duration for new frames
  bool is_looping = true;

  // Selection state for frame list
  int selected_frame_index = -1;        // Currently selected frame in the list
  int drag_source_index = -1;           // Frame being dragged for reorder

  // Atlas preview state
  ImGuiWidgets::TextureAtlasState atlas_state;
  bool was_dragging_last_frame = false;  // Track drag state to detect click completion

  // Animation preview state
  bool is_previewing = false;
  float preview_start_time = 0.0f;

  // Texture and atlas cache (updated when state changes)
  Components::Texture cached_texture;
  Components::TextureAtlas cached_atlas;
  bool cache_valid = false;

  // Helper to get the state animation from the entity
  Components::StateAnimation* get_state_animation(Engine::ECS::Entity entity, const std::string& state_name) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    if (!state_machine) return nullptr;

    auto it = state_machine->states.find(state_name);
    if (it == state_machine->states.end()) return nullptr;

    if (!it->second.animation.has_value()) return nullptr;

    return &it->second.animation.value();
  }

  // Load frame data from the state's animation
  void load_from_state() {
    if (selected_entity_id < 0 || selected_state_name.empty()) {
      cache_valid = false;
      return;
    }

    Engine::ECS::Entity entity(static_cast<u32>(selected_entity_id));
    if (!entity.is_valid()) {
      cache_valid = false;
      return;
    }

    auto* state_anim = get_state_animation(entity, selected_state_name);
    if (!state_anim) {
      cache_valid = false;
      return;
    }

    // Cache texture and atlas
    cached_texture = state_anim->texture;
    cached_atlas = state_anim->texture_atlas;

    // Load frame indices and durations
    frame_indices.clear();
    frame_durations.clear();

    const auto& frames = state_anim->animation.frames;
    for (const auto& frame : frames) {
      frame_indices.push_back(frame.atlas_index);
      frame_durations.push_back(frame.duration);
    }

    is_looping = state_anim->animation.is_looping;

    // Set default duration based on first frame if available
    if (!frame_durations.empty()) {
      default_duration = frame_durations[0];
    }

    cache_valid = true;
    selected_frame_index = -1;
    atlas_state.reset();
  }

  // Apply frame data back to the state's animation
  void apply_to_state() {
    if (selected_entity_id < 0 || selected_state_name.empty()) return;

    Engine::ECS::Entity entity(static_cast<u32>(selected_entity_id));
    if (!entity.is_valid()) return;

    auto* state_anim = get_state_animation(entity, selected_state_name);
    if (!state_anim) return;

    // Use the Animation constructor with custom indices and durations
    state_anim->animation = Components::Animation(frame_indices, frame_durations, is_looping);

    Log::info("Applied animation changes: {} frames to state '{}'",
      frame_indices.size(), selected_state_name);
  }

  // Get the current preview frame index
  int get_preview_frame_index() {
    if (frame_indices.empty() || frame_durations.empty()) return 0;

    float elapsed = Engine::Time::get_time() - preview_start_time;
    float total_duration = 0.0f;
    for (float d : frame_durations) total_duration += d;

    if (total_duration <= 0.0f) return 0;

    float time_to_use = is_looping ? fmod(elapsed, total_duration) : std::min(elapsed, total_duration);

    float accumulated = 0.0f;
    for (size_t i = 0; i < frame_durations.size(); ++i) {
      accumulated += frame_durations[i];
      if (time_to_use < accumulated) {
        return static_cast<int>(i);
      }
    }
    return static_cast<int>(frame_indices.size() - 1);
  }

  // Draw a single frame thumbnail from the atlas
  void draw_frame_thumbnail(ImDrawList* draw_list, ImVec2 pos, ImVec2 size, int atlas_index,
    const Components::Texture& texture, const Components::TextureAtlas& atlas) {
    if (atlas_index < 0 || atlas_index >= static_cast<int>(atlas.subTextures.size())) {
      draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(60, 60, 60, 255));
      return;
    }

    const auto& sub = atlas.subTextures[atlas_index];
    float min_u = sub.tex_coords[3].x; // top-left U
    float min_v = sub.tex_coords[3].y; // top-left V
    float max_u = sub.tex_coords[1].x; // bottom-right U
    float max_v = sub.tex_coords[1].y; // bottom-right V

    draw_list->AddImage(
      (ImTextureID)(intptr_t)texture.texture_handle->id,
      pos,
      ImVec2(pos.x + size.x, pos.y + size.y),
      ImVec2(min_u, min_v),
      ImVec2(max_u, max_v)
    );
  }

} // anonymous namespace

namespace FrameAnimationEditor {

  void Open(Engine::ECS::Entity entity, const std::string& state_name) {
    selected_entity_id = static_cast<s32>(entity.get_id());
    selected_state_name = state_name;
    is_open = true;
    is_previewing = false;
    load_from_state();
  }

  void Close() {
    is_open = false;
    selected_entity_id = -1;
    selected_state_name = "";
    cache_valid = false;
    frame_indices.clear();
    frame_durations.clear();
    atlas_state.reset();
  }

  bool IsOpen() {
    return is_open;
  }

} // namespace FrameAnimationEditor

void FrameAnimationEditorPanel() {
  if (!is_open) return;

  ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);

  std::string window_title = "Frame Animation Editor";
  if (!selected_state_name.empty()) {
    window_title += " - " + selected_state_name;
  }
  window_title += "###FrameAnimationEditor";

  if (!ImGui::Begin(window_title.c_str(), &is_open)) {
    ImGui::End();
    return;
  }

  // Validate entity and state
  if (selected_entity_id < 0) {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No entity selected");
    ImGui::End();
    return;
  }

  Engine::ECS::Entity entity(static_cast<u32>(selected_entity_id));
  if (!entity.is_valid()) {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Invalid entity");
    ImGui::End();
    return;
  }

  auto* state_anim = get_state_animation(entity, selected_state_name);
  if (!state_anim) {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "State '%s' has no animation data", selected_state_name.c_str());
    ImGui::End();
    return;
  }

  if (!cache_valid) {
    load_from_state();
  }

  // ============================================================================
  // Header: State info and action buttons
  // ============================================================================
  ImGui::Text("Editing animation for state: ");
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", selected_state_name.c_str());

  ImGui::SameLine(ImGui::GetWindowWidth() - 200);
  if (ImGui::Button("Apply Changes", ImVec2(90, 0))) {
    apply_to_state();
  }
  ImGui::SameLine();
  if (ImGui::Button("Revert", ImVec2(70, 0))) {
    load_from_state();
  }

  ImGui::Separator();

  // ============================================================================
  // Left side: Texture Atlas Preview
  // ============================================================================
  ImGui::BeginChild("AtlasPanel", ImVec2(350, 0), true);

  ImGui::Text("Texture Atlas");
  ImGui::SameLine();
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Click tiles to add them to the animation.\nHold Ctrl+Click to add multiple frames.\nShift+Click for range selection.");
  }

  ImGui::Separator();

  if (cached_texture.texture_handle && cached_texture.texture_handle->id != 0 && !cached_atlas.subTextures.empty()) {
    // Calculate preview size to fit in panel
    float available_width = ImGui::GetContentRegionAvail().x - 10;
    float aspect = static_cast<float>(cached_texture.texture_handle->width) /
      static_cast<float>(cached_texture.texture_handle->height);
    float preview_height = std::min(300.0f, available_width / aspect);

    // Draw atlas with widget
    ImGuiWidgets::TextureAtlasPreview(cached_texture, cached_atlas, preview_height, atlas_state);

    // Detect when a click/drag just completed (was dragging, now not dragging)
    bool click_completed = was_dragging_last_frame && !atlas_state.is_dragging;
    was_dragging_last_frame = atlas_state.is_dragging;

    // Check if user just finished selecting tiles
    if (click_completed && !atlas_state.current_selection.is_empty()) {

      bool ctrl_held = ImGui::GetIO().KeyCtrl;
      bool shift_held = ImGui::GetIO().KeyShift;

      if (shift_held && selected_frame_index >= 0 && !frame_indices.empty()) {
        // Range selection: add all tiles from last selected to current
        int start_atlas_idx = frame_indices[selected_frame_index];
        for (int idx : atlas_state.current_selection.indices) {
          int lo = std::min(start_atlas_idx, idx);
          int hi = std::max(start_atlas_idx, idx);
          for (int i = lo; i <= hi; ++i) {
            frame_indices.push_back(i);
            frame_durations.push_back(default_duration);
          }
        }
      }
      else if (ctrl_held) {
        // Multi-select: add to existing frames
        for (int idx : atlas_state.current_selection.indices) {
          frame_indices.push_back(idx);
          frame_durations.push_back(default_duration);
        }
      }
      else {
        // Single click: add frame(s)
        for (int idx : atlas_state.current_selection.indices) {
          frame_indices.push_back(idx);
          frame_durations.push_back(default_duration);
        }
      }
    }

    ImGui::Spacing();

    // Atlas info
    ImGui::TextDisabled("Atlas: %d tiles | Texture: %dx%d",
      static_cast<int>(cached_atlas.subTextures.size()),
      cached_texture.texture_handle->width,
      cached_texture.texture_handle->height);
  }
  else {
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No texture/atlas data");
  }

  ImGui::EndChild();

  ImGui::SameLine();

  // ============================================================================
  // Right side: Frame List and Settings
  // ============================================================================
  ImGui::BeginChild("FramePanel", ImVec2(0, 0), true);

  // Animation settings header
  ImGui::Text("Animation Frames");
  ImGui::SameLine();
  ImGui::TextDisabled("(%d frames)", static_cast<int>(frame_indices.size()));

  ImGui::Separator();

  // Global settings
  ImGui::Checkbox("Looping", &is_looping);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(100);
  ImGui::DragFloat("Default Duration", &default_duration, 0.01f, 0.01f, 10.0f, "%.3f s");

  ImGui::SameLine(ImGui::GetContentRegionAvail().x - 140);

  // Set all durations button
  if (ImGui::Button("Set All", ImVec2(60, 0))) {
    for (float& d : frame_durations) {
      d = default_duration;
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear All", ImVec2(70, 0))) {
    frame_indices.clear();
    frame_durations.clear();
    selected_frame_index = -1;
  }

  ImGui::Separator();

  // ============================================================================
  // Frame List with thumbnails
  // ============================================================================
  float frame_list_height = ImGui::GetContentRegionAvail().y - 120; // Reserve space for preview
  ImGui::BeginChild("FrameList", ImVec2(0, frame_list_height), true);

  const float thumbnail_size = 48.0f;
  const float item_height = thumbnail_size + 8.0f;

  for (int i = 0; i < static_cast<int>(frame_indices.size()); ++i) {
    ImGui::PushID(i);

    // Frame item background
    bool is_selected = (i == selected_frame_index);
    bool is_preview_frame = is_previewing && (i == get_preview_frame_index());

    ImVec2 item_pos = ImGui::GetCursorScreenPos();
    ImVec2 item_size(ImGui::GetContentRegionAvail().x, item_height);

    // Highlight background
    ImU32 bg_color = IM_COL32(40, 40, 40, 255);
    if (is_selected) bg_color = IM_COL32(60, 80, 120, 255);
    if (is_preview_frame) bg_color = IM_COL32(80, 120, 60, 255);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(item_pos, ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y), bg_color);

    // Selectable overlay for interaction
    if (ImGui::Selectable("##frame", is_selected, ImGuiSelectableFlags_AllowOverlap, item_size)) {
      selected_frame_index = i;
    }

    // Drag source for reordering
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
      drag_source_index = i;
      ImGui::SetDragDropPayload("FRAME_REORDER", &i, sizeof(int));
      ImGui::Text("Frame %d (Atlas: %d)", i, frame_indices[i]);
      ImGui::EndDragDropSource();
    }

    // Drop target for reordering
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FRAME_REORDER")) {
        int source = *(const int*)payload->Data;
        if (source != i) {
          // Reorder frames
          int idx_to_move = frame_indices[source];
          float dur_to_move = frame_durations[source];

          frame_indices.erase(frame_indices.begin() + source);
          frame_durations.erase(frame_durations.begin() + source);

          int dest = (source < i) ? i - 1 : i;
          frame_indices.insert(frame_indices.begin() + dest, idx_to_move);
          frame_durations.insert(frame_durations.begin() + dest, dur_to_move);

          selected_frame_index = dest;
        }
      }
      ImGui::EndDragDropTarget();
    }

    // Draw content on same line
    ImGui::SameLine(8);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - item_height + 4);

    // Thumbnail
    ImVec2 thumb_pos = ImGui::GetCursorScreenPos();
    draw_frame_thumbnail(draw_list, thumb_pos, ImVec2(thumbnail_size, thumbnail_size),
      frame_indices[i], cached_texture, cached_atlas);
    draw_list->AddRect(thumb_pos, ImVec2(thumb_pos.x + thumbnail_size, thumb_pos.y + thumbnail_size),
      IM_COL32(100, 100, 100, 255));

    // Frame info and duration editor
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + thumbnail_size + 12);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);

    ImGui::Text("Frame %d", i);
    ImGui::SameLine();
    ImGui::TextDisabled("(Atlas: %d)", frame_indices[i]);

    // Duration slider
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + thumbnail_size + 12);
    ImGui::SetNextItemWidth(120);
    ImGui::DragFloat("##dur", &frame_durations[i], 0.005f, 0.01f, 10.0f, "%.3f s");

    // Delete button
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 10);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("X##del", ImVec2(24, 24))) {
      frame_indices.erase(frame_indices.begin() + i);
      frame_durations.erase(frame_durations.begin() + i);
      if (selected_frame_index >= static_cast<int>(frame_indices.size())) {
        selected_frame_index = static_cast<int>(frame_indices.size()) - 1;
      }
      ImGui::PopStyleColor();
      ImGui::PopID();
      continue; // Skip to next iteration
    }
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY(item_pos.y - ImGui::GetWindowPos().y + ImGui::GetScrollY() + item_height + 4);

    ImGui::PopID();
  }

  if (frame_indices.empty()) {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No frames added yet.\nClick tiles in the atlas to add frames.");
  }

  ImGui::EndChild();

  ImGui::Separator();

  // ============================================================================
  // Animation Preview Section
  // ============================================================================
  ImGui::Text("Preview");
  ImGui::SameLine();

  // Play/Stop button
  if (is_previewing) {
    if (ImGui::Button("Stop", ImVec2(60, 0))) {
      is_previewing = false;
    }
  }
  else {
    if (ImGui::Button("Play", ImVec2(60, 0))) {
      is_previewing = true;
      preview_start_time = Engine::Time::get_time();
    }
  }

  ImGui::SameLine();

  // Calculate total duration for display
  float total_duration = 0.0f;
  for (float d : frame_durations) total_duration += d;
  ImGui::TextDisabled("Total: %.2f s", total_duration);

  // Preview thumbnail strip
  ImGui::SameLine(200);
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  ImVec2 preview_pos = ImGui::GetCursorScreenPos();
  const float preview_thumb_size = 40.0f;
  const float preview_spacing = 4.0f;

  int current_preview_frame = is_previewing ? get_preview_frame_index() : -1;

  // Show up to 8 frames in the strip
  int max_preview_frames = std::min(8, static_cast<int>(frame_indices.size()));
  for (int i = 0; i < max_preview_frames; ++i) {
    ImVec2 pos(preview_pos.x + i * (preview_thumb_size + preview_spacing), preview_pos.y);
    draw_frame_thumbnail(draw_list, pos, ImVec2(preview_thumb_size, preview_thumb_size),
      frame_indices[i], cached_texture, cached_atlas);

    // Highlight current preview frame
    ImU32 border_color = (i == current_preview_frame) ?
      IM_COL32(0, 255, 0, 255) : IM_COL32(100, 100, 100, 255);
    float thickness = (i == current_preview_frame) ? 3.0f : 1.0f;
    draw_list->AddRect(pos, ImVec2(pos.x + preview_thumb_size, pos.y + preview_thumb_size),
      border_color, 0.0f, 0, thickness);
  }

  if (frame_indices.size() > 8) {
    ImGui::SetCursorPosX(preview_pos.x - ImGui::GetWindowPos().x + 8 * (preview_thumb_size + preview_spacing));
    ImGui::TextDisabled("+%d more", static_cast<int>(frame_indices.size()) - 8);
  }

  // Dummy to reserve space
  ImGui::Dummy(ImVec2(0, preview_thumb_size + 10));

  ImGui::EndChild();

  ImGui::End();

  // Handle window close
  if (!is_open) {
    FrameAnimationEditor::Close();
  }
}
