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
//          animation frames. Provides timeline-based frame editing with drag-reorder,
//          duration adjustment, and live animation preview.
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

// Global wrapper functions for extern declarations in ReflectionFieldWidgets.cpp
namespace ImGuiWidgets {
  void FrameAnimationEditor_Open(Engine::ECS::Entity entity, const std::string& state_name) {
    FrameAnimationEditor::Open(entity, state_name);
  }

  void FrameAnimationEditor_OpenForAnimation(Engine::ECS::Entity entity) {
    FrameAnimationEditor::OpenForAnimation(entity);
  }

  // Wrapper with prefixed name for extern declaration inside ImGuiWidgets namespace
  void ImGuiWidgets_FrameAnimationEditor_OpenForAnimation(Engine::ECS::Entity entity) {
    FrameAnimationEditor::OpenForAnimation(entity);
  }
}

namespace {
  // Editor mode
  enum class EditorMode {
    StateMachine,  // Editing a state's animation within a StateMachine
    Standalone     // Editing an entity's Animation component directly
  };

  // Editor state
  bool is_open = false;
  s32 selected_entity_id = -1;
  std::string selected_state_name = "";
  EditorMode editor_mode = EditorMode::StateMachine;

  // Frame editing state
  std::vector<int> frame_indices;       // Selected frame indices from atlas
  std::vector<float> frame_durations;   // Duration for each frame
  float default_duration = 0.1f;        // Default duration for new frames
  bool is_looping = true;

  // Timeline state
  int selected_frame_index = -1;        // Currently selected frame in timeline
  int dragging_frame_index = -1;        // Frame being dragged for reorder
  int drop_target_index = -1;           // Where the dragged frame would be inserted
  float timeline_zoom = 1.0f;           // Timeline zoom level
  float timeline_scroll = 0.0f;         // Timeline horizontal scroll

  // Panel layout state
  float atlas_panel_width = 300.0f;     // Resizable atlas panel width
  float atlas_zoom = 1.0f;              // Atlas preview zoom level

  // Atlas preview state
  ImGuiWidgets::TextureAtlasState atlas_state;
  bool was_dragging_last_frame = false;

  // Animation preview state
  bool is_previewing = false;
  float preview_start_time = 0.0f;

  // Texture and atlas cache
  Components::Texture cached_texture;
  Components::TextureAtlas cached_atlas;
  bool cache_valid = false;

  // Constants
  const float TIMELINE_HEIGHT = 80.0f;
  const float TIMELINE_FRAME_MIN_WIDTH = 8.0f;  // Allow frames to shrink very small
  const float TIMELINE_FRAME_HEIGHT = 60.0f;
  const float PIXELS_PER_SECOND = 200.0f;

  // Helper to get the state animation from the entity
  Components::StateAnimation* get_state_animation(Engine::ECS::Entity entity, const std::string& state_name) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    if (!state_machine) return nullptr;

    auto it = state_machine->states.find(state_name);
    if (it == state_machine->states.end()) return nullptr;

    if (!it->second.animation.has_value()) return nullptr;

    return &it->second.animation.value();
  }

  // Load frame data from the animation source (StateMachine state or standalone Animation)
  void load_from_source() {
    if (selected_entity_id < 0) {
      cache_valid = false;
      return;
    }

    Engine::ECS::Entity entity(static_cast<u32>(selected_entity_id));
    if (!entity.is_valid()) {
      cache_valid = false;
      return;
    }

    frame_indices.clear();
    frame_durations.clear();

    if (editor_mode == EditorMode::StateMachine) {
      // Load from StateMachine state
      if (selected_state_name.empty()) {
        cache_valid = false;
        return;
      }

      auto* state_anim = get_state_animation(entity, selected_state_name);
      if (!state_anim) {
        cache_valid = false;
        return;
      }

      // Cache texture and atlas from state
      cached_texture = state_anim->texture;
      cached_atlas = state_anim->texture_atlas;

      // Load frame indices and durations
      const auto& frames = state_anim->animation.frames;
      for (const auto& frame : frames) {
        frame_indices.push_back(frame.atlas_index);
        frame_durations.push_back(frame.duration);
      }

      is_looping = state_anim->animation.is_looping;
    }
    else {
      // Load from standalone Animation component
      auto* animation = entity.get_component<Components::Animation>();
      auto* texture = entity.get_component<Components::Texture>();
      auto* atlas = entity.get_component<Components::TextureAtlas>();

      if (!animation || !texture || !atlas) {
        cache_valid = false;
        return;
      }

      // Cache texture and atlas from entity components
      cached_texture = *texture;
      cached_atlas = *atlas;

      // Load frame indices and durations
      const auto& frames = animation->frames;
      for (const auto& frame : frames) {
        frame_indices.push_back(frame.atlas_index);
        frame_durations.push_back(frame.duration);
      }

      is_looping = animation->is_looping;
    }

    if (!frame_durations.empty()) {
      default_duration = frame_durations[0];
    }

    cache_valid = true;
    selected_frame_index = -1;
    atlas_state.reset();
  }

  // Apply frame data back to the animation source
  void apply_to_source() {
    if (selected_entity_id < 0) return;

    Engine::ECS::Entity entity(static_cast<u32>(selected_entity_id));
    if (!entity.is_valid()) return;

    if (editor_mode == EditorMode::StateMachine) {
      if (selected_state_name.empty()) return;

      auto* state_anim = get_state_animation(entity, selected_state_name);
      if (!state_anim) return;

      state_anim->animation = Components::Animation(frame_indices, frame_durations, is_looping);

      Log::info("Applied animation changes: {} frames to state '{}'",
        frame_indices.size(), selected_state_name);
    }
    else {
      auto* animation = entity.get_component<Components::Animation>();
      if (!animation) return;

      *animation = Components::Animation(frame_indices, frame_durations, is_looping);

      Log::info("Applied animation changes: {} frames to Animation component",
        frame_indices.size());
    }
  }

  // Get the current preview frame index
  int get_preview_frame_index() {
    if (frame_indices.empty() || frame_durations.empty()) return -1;

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

  // Draw a frame thumbnail
  void draw_frame_thumbnail(ImDrawList* draw_list, ImVec2 pos, ImVec2 size, int atlas_index) {
    if (!cached_texture.texture_handle || atlas_index < 0 ||
      atlas_index >= static_cast<int>(cached_atlas.subTextures.size())) {
      draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(60, 60, 60, 255));
      return;
    }

    const auto& sub = cached_atlas.subTextures[atlas_index];
    float min_u = sub.tex_coords[3].x;
    float min_v = sub.tex_coords[3].y;
    float max_u = sub.tex_coords[1].x;
    float max_v = sub.tex_coords[1].y;

    draw_list->AddImage(
      (ImTextureID)(intptr_t)cached_texture.texture_handle->id,
      pos, ImVec2(pos.x + size.x, pos.y + size.y),
      ImVec2(min_u, min_v), ImVec2(max_u, max_v)
    );
  }

  // Draw the timeline
  void draw_timeline() {
    ImGui::Text("Timeline");
    ImGui::SameLine();

    // Zoom controls
    ImGui::SameLine(ImGui::GetWindowWidth() - 200);
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("Zoom", &timeline_zoom, 0.1f, 10.0f, "%.2fx");

    ImGui::SameLine();
    if (ImGui::Button("Fit")) {
      float total_duration = 0.0f;
      for (float d : frame_durations) total_duration += d;
      if (total_duration > 0.0f) {
        float available_width = ImGui::GetContentRegionAvail().x - 20;
        timeline_zoom = available_width / (total_duration * PIXELS_PER_SECOND);
        timeline_zoom = std::max(0.01f, timeline_zoom); // Only enforce minimum
      }
    }

    // Timeline area
    ImVec2 timeline_pos = ImGui::GetCursorScreenPos();
    ImVec2 timeline_size(ImGui::GetContentRegionAvail().x, TIMELINE_HEIGHT);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Background
    draw_list->AddRectFilled(timeline_pos,
      ImVec2(timeline_pos.x + timeline_size.x, timeline_pos.y + timeline_size.y),
      IM_COL32(30, 30, 35, 255));

    // Border
    draw_list->AddRect(timeline_pos,
      ImVec2(timeline_pos.x + timeline_size.x, timeline_pos.y + timeline_size.y),
      IM_COL32(60, 60, 70, 255));

    // Set clip rect for timeline content
    draw_list->PushClipRect(timeline_pos,
      ImVec2(timeline_pos.x + timeline_size.x, timeline_pos.y + timeline_size.y), true);

    // Draw time markers
    float time_step = 0.1f; // 100ms markers
    float total_duration = 0.0f;
    for (float d : frame_durations) total_duration += d;

    for (float t = 0.0f; t <= total_duration + 0.01f; t += time_step) {
      float x = timeline_pos.x + t * PIXELS_PER_SECOND * timeline_zoom - timeline_scroll;
      if (x >= timeline_pos.x && x <= timeline_pos.x + timeline_size.x) {
        bool is_major = fmod(t, 0.5f) < 0.01f;
        float line_height = is_major ? 12.0f : 6.0f;
        draw_list->AddLine(
          ImVec2(x, timeline_pos.y),
          ImVec2(x, timeline_pos.y + line_height),
          IM_COL32(100, 100, 110, 255), 1.0f
        );
        if (is_major) {
          char time_str[16];
          snprintf(time_str, sizeof(time_str), "%.1fs", t);
          draw_list->AddText(ImVec2(x + 2, timeline_pos.y + 2), IM_COL32(150, 150, 160, 255), time_str);
        }
      }
    }

    // Draw frames on timeline
    float frame_y = timeline_pos.y + 18.0f;
    float current_x = timeline_pos.x - timeline_scroll;

    ImVec2 mouse_pos = ImGui::GetMousePos();
    bool mouse_in_timeline = mouse_pos.x >= timeline_pos.x &&
      mouse_pos.x <= timeline_pos.x + timeline_size.x &&
      mouse_pos.y >= timeline_pos.y &&
      mouse_pos.y <= timeline_pos.y + timeline_size.y;

    drop_target_index = -1;

    for (int i = 0; i < static_cast<int>(frame_indices.size()); ++i) {
      float frame_width = std::max(TIMELINE_FRAME_MIN_WIDTH, frame_durations[i] * PIXELS_PER_SECOND * timeline_zoom);

      ImVec2 frame_pos(current_x, frame_y);
      ImVec2 frame_end(current_x + frame_width - 2, frame_y + TIMELINE_FRAME_HEIGHT);

      // Check if this is where we'd drop
      if (dragging_frame_index >= 0 && dragging_frame_index != i && mouse_in_timeline) {
        float frame_center = current_x + frame_width * 0.5f;
        if (mouse_pos.x < frame_center && (drop_target_index < 0 || i < drop_target_index)) {
          drop_target_index = i;
        }
      }

      // Only draw if visible
      if (frame_end.x >= timeline_pos.x && frame_pos.x <= timeline_pos.x + timeline_size.x) {
        bool is_selected = (i == selected_frame_index);
        bool is_dragging = (i == dragging_frame_index);
        bool is_preview_frame = is_previewing && (i == get_preview_frame_index());

        // Frame background
        ImU32 bg_color = IM_COL32(50, 55, 65, 255);
        if (is_selected) bg_color = IM_COL32(70, 100, 140, 255);
        if (is_preview_frame) bg_color = IM_COL32(80, 130, 80, 255);
        if (is_dragging) bg_color = IM_COL32(100, 100, 60, 200);

        draw_list->AddRectFilled(frame_pos, frame_end, bg_color, 4.0f);

        // Frame thumbnail (centered) - allow shrinking to any size
        float thumb_size = std::min(frame_width - 4, TIMELINE_FRAME_HEIGHT - 20);
        if (thumb_size > 4.0f) {
          ImVec2 thumb_pos(
            frame_pos.x + (frame_width - thumb_size) * 0.5f - 1,
            frame_pos.y + 4
          );
          draw_frame_thumbnail(draw_list, thumb_pos, ImVec2(thumb_size, thumb_size), frame_indices[i]);
        }

        // Frame index label - only show if there's enough space
        if (frame_width > 20.0f) {
          char label[32];
          snprintf(label, sizeof(label), "%d", frame_indices[i]);
          ImVec2 label_size = ImGui::CalcTextSize(label);
          draw_list->AddText(
            ImVec2(frame_pos.x + (frame_width - label_size.x) * 0.5f - 1, frame_end.y - 14),
            IM_COL32(200, 200, 200, 255), label
          );
        }

        // Duration label at bottom - only show if there's enough space
        if (frame_width > 30.0f && thumb_size > 10.0f) {
          char dur_label[16];
          snprintf(dur_label, sizeof(dur_label), "%.2fs", frame_durations[i]);
          ImVec2 dur_size = ImGui::CalcTextSize(dur_label);
          draw_list->AddText(
            ImVec2(frame_pos.x + (frame_width - dur_size.x) * 0.5f - 1, frame_pos.y + thumb_size + 6),
            IM_COL32(150, 150, 160, 255), dur_label
          );
        }

        // Border
        ImU32 border_color = is_selected ? IM_COL32(100, 160, 220, 255) : IM_COL32(80, 85, 95, 255);
        if (is_preview_frame) border_color = IM_COL32(100, 200, 100, 255);
        draw_list->AddRect(frame_pos, frame_end, border_color, 4.0f, 0, is_selected ? 2.0f : 1.0f);

        // Handle clicking and dragging
        ImGui::SetCursorScreenPos(frame_pos);
        ImGui::InvisibleButton(("frame_btn_" + std::to_string(i)).c_str(),
          ImVec2(frame_width - 2, TIMELINE_FRAME_HEIGHT));

        if (ImGui::IsItemClicked(0)) {
          selected_frame_index = i;
        }

        // Middle click to delete frame
        if (ImGui::IsItemClicked(2)) {
          frame_indices.erase(frame_indices.begin() + i);
          frame_durations.erase(frame_durations.begin() + i);
          if (selected_frame_index >= static_cast<int>(frame_indices.size())) {
            selected_frame_index = static_cast<int>(frame_indices.size()) - 1;
          }
          draw_list->PopClipRect();
          ImGui::Dummy(timeline_size);
          return; // Exit early since we modified the array
        }

        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0, 5.0f)) {
          dragging_frame_index = i;
        }
      }

      current_x += frame_width;
    }

    // Handle end of timeline for drop target
    if (dragging_frame_index >= 0 && mouse_in_timeline && drop_target_index < 0) {
      drop_target_index = static_cast<int>(frame_indices.size());
    }

    // Draw drop indicator
    if (dragging_frame_index >= 0 && drop_target_index >= 0 && drop_target_index != dragging_frame_index) {
      float indicator_x = timeline_pos.x - timeline_scroll;
      for (int i = 0; i < drop_target_index && i < static_cast<int>(frame_indices.size()); ++i) {
        indicator_x += std::max(TIMELINE_FRAME_MIN_WIDTH, frame_durations[i] * PIXELS_PER_SECOND * timeline_zoom);
      }
      draw_list->AddLine(
        ImVec2(indicator_x, frame_y - 2),
        ImVec2(indicator_x, frame_y + TIMELINE_FRAME_HEIGHT + 2),
        IM_COL32(255, 200, 50, 255), 3.0f
      );
    }

    // Handle drop
    if (dragging_frame_index >= 0 && ImGui::IsMouseReleased(0)) {
      if (drop_target_index >= 0 && drop_target_index != dragging_frame_index &&
        drop_target_index != dragging_frame_index + 1) {
        // Perform reorder
        int idx_to_move = frame_indices[dragging_frame_index];
        float dur_to_move = frame_durations[dragging_frame_index];

        frame_indices.erase(frame_indices.begin() + dragging_frame_index);
        frame_durations.erase(frame_durations.begin() + dragging_frame_index);

        int insert_pos = (dragging_frame_index < drop_target_index) ? drop_target_index - 1 : drop_target_index;
        frame_indices.insert(frame_indices.begin() + insert_pos, idx_to_move);
        frame_durations.insert(frame_durations.begin() + insert_pos, dur_to_move);

        selected_frame_index = insert_pos;
      }
      dragging_frame_index = -1;
    }

    // Pop clip rect before drawing playhead (so it's not clipped)
    draw_list->PopClipRect();

    // Handle horizontal scrolling and zooming with mouse wheel when hovering timeline
    ImGui::SetCursorScreenPos(timeline_pos);
    ImGui::InvisibleButton("timeline_scroll_area", timeline_size);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Scroll: Mouse wheel\nCtrl+Scroll: Zoom\nMiddle-click frame: Delete");
      float wheel = ImGui::GetIO().MouseWheel;
      if (wheel != 0.0f) {
        bool ctrl_held = ImGui::GetIO().KeyCtrl;

        if (ctrl_held) {
          // Ctrl+scroll: Zoom in/out (uncapped)
          float zoom_factor = 1.0f + wheel * 0.15f;
          timeline_zoom *= zoom_factor;
          timeline_zoom = std::max(0.01f, timeline_zoom); // Only enforce minimum to prevent zero/negative
        }
        else {
          // Normal scroll: Horizontal pan
          // Calculate total timeline width
          float total_width = 0.0f;
          for (float d : frame_durations) {
            total_width += std::max(TIMELINE_FRAME_MIN_WIDTH, d * PIXELS_PER_SECOND * timeline_zoom);
          }
          float max_scroll = std::max(0.0f, total_width - timeline_size.x + 20.0f);

          timeline_scroll -= wheel * 50.0f; // Scroll speed
          timeline_scroll = std::clamp(timeline_scroll, 0.0f, max_scroll);
        }
      }
    }

    // Reserve space for timeline
    ImGui::Dummy(timeline_size);

    // Playhead for preview
    if (is_previewing) {
      float elapsed = Engine::Time::get_time() - preview_start_time;
      float total_dur = 0.0f;
      for (float d : frame_durations) total_dur += d;
      if (total_dur > 0.0f) {
        float time_pos = is_looping ? fmod(elapsed, total_dur) : std::min(elapsed, total_dur);
        float playhead_x = timeline_pos.x + time_pos * PIXELS_PER_SECOND * timeline_zoom - timeline_scroll;
        if (playhead_x >= timeline_pos.x && playhead_x <= timeline_pos.x + timeline_size.x) {
          draw_list->AddLine(
            ImVec2(playhead_x, timeline_pos.y),
            ImVec2(playhead_x, timeline_pos.y + timeline_size.y),
            IM_COL32(255, 80, 80, 255), 2.0f
          );
          // Playhead triangle
          draw_list->AddTriangleFilled(
            ImVec2(playhead_x - 6, timeline_pos.y),
            ImVec2(playhead_x + 6, timeline_pos.y),
            ImVec2(playhead_x, timeline_pos.y + 8),
            IM_COL32(255, 80, 80, 255)
          );
        }
      }
    }
  }

} // anonymous namespace

namespace FrameAnimationEditor {

  void Open(Engine::ECS::Entity entity, const std::string& state_name) {
    selected_entity_id = static_cast<s32>(entity.get_id());
    selected_state_name = state_name;
    editor_mode = EditorMode::StateMachine;
    is_open = true;
    is_previewing = false;
    dragging_frame_index = -1;
    load_from_source();
  }

  void OpenForAnimation(Engine::ECS::Entity entity) {
    selected_entity_id = static_cast<s32>(entity.get_id());
    selected_state_name = "";
    editor_mode = EditorMode::Standalone;
    is_open = true;
    is_previewing = false;
    dragging_frame_index = -1;
    load_from_source();
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

  ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);

  std::string window_title = "Frame Animation Editor";
  if (editor_mode == EditorMode::StateMachine && !selected_state_name.empty()) {
    window_title += " - " + selected_state_name;
  }
  else if (editor_mode == EditorMode::Standalone) {
    window_title += " - Animation Component";
  }
  window_title += "###FrameAnimationEditor";

  if (!ImGui::Begin(window_title.c_str(), &is_open)) {
    ImGui::End();
    return;
  }

  // Validate
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

  // Mode-specific validation
  if (editor_mode == EditorMode::StateMachine) {
    auto* state_anim = get_state_animation(entity, selected_state_name);
    if (!state_anim) {
      ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "State '%s' has no animation data", selected_state_name.c_str());
      ImGui::End();
      return;
    }
  }
  else {
    // Standalone mode - check for Animation, Texture, and TextureAtlas components
    if (!entity.has_component<Components::Animation>()) {
      ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Entity has no Animation component");
      ImGui::End();
      return;
    }
    if (!entity.has_component<Components::Texture>() || !entity.has_component<Components::TextureAtlas>()) {
      ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Entity needs Texture and TextureAtlas components");
      ImGui::End();
      return;
    }
  }

  if (!cache_valid) {
    load_from_source();
  }

  // ============================================================================
  // Header: Controls
  // ============================================================================
  // Play/Stop
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
  ImGui::Checkbox("Loop", &is_looping);

  ImGui::SameLine();
  float total_duration = 0.0f;
  for (float d : frame_durations) total_duration += d;
  ImGui::Text("| %d frames | %.2fs total", static_cast<int>(frame_indices.size()), total_duration);

  ImGui::SameLine(ImGui::GetWindowWidth() - 180);
  if (ImGui::Button("Apply", ImVec2(80, 0))) {
    apply_to_source();
  }
  ImGui::SameLine();
  if (ImGui::Button("Revert", ImVec2(80, 0))) {
    load_from_source();
  }

  ImGui::Separator();

  // ============================================================================
  // Main content: Atlas on left, Frame properties on right
  // ============================================================================
  float panel_height = ImGui::GetContentRegionAvail().y - TIMELINE_HEIGHT - 30;
  float total_width = ImGui::GetContentRegionAvail().x;

  // Clamp atlas panel width
  atlas_panel_width = std::clamp(atlas_panel_width, 150.0f, total_width - 200.0f);

  // Left: Texture Atlas
  ImGui::BeginChild("AtlasPanel", ImVec2(atlas_panel_width, panel_height), true, ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::Text("Texture Atlas");
  ImGui::SameLine();
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Click a tile to add it to the timeline\nCtrl+Scroll: Zoom");
  }
  ImGui::Separator();

  // Handle Ctrl+scroll zoom for atlas
  if (ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl) {
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0.0f) {
      float zoom_factor = 1.0f + wheel * 0.15f;
      atlas_zoom *= zoom_factor;
      atlas_zoom = std::max(0.1f, atlas_zoom); // Minimum zoom
    }
  }

  if (cached_texture.texture_handle && cached_texture.texture_handle->id != 0 && !cached_atlas.subTextures.empty()) {
    float available_width = ImGui::GetContentRegionAvail().x - 10;
    float aspect = static_cast<float>(cached_texture.texture_handle->width) /
      static_cast<float>(cached_texture.texture_handle->height);
    constexpr float MIN_ATLAS_HEIGHT = 150.0f;
    // Apply zoom to the preview height
    float base_height = std::max(MIN_ATLAS_HEIGHT, available_width / aspect);
    float preview_height = base_height * atlas_zoom;

    ImGuiWidgets::TextureAtlasPreview(cached_texture, cached_atlas, preview_height, atlas_state);

    bool click_completed = was_dragging_last_frame && !atlas_state.is_dragging;
    was_dragging_last_frame = atlas_state.is_dragging;

    if (click_completed && !atlas_state.current_selection.is_empty()) {
      for (int idx : atlas_state.current_selection.indices) {
        frame_indices.push_back(idx);
        frame_durations.push_back(default_duration);
      }
      // Select the newly added frame
      selected_frame_index = static_cast<int>(frame_indices.size()) - 1;
    }

    ImGui::Spacing();
    ImGui::TextDisabled("%d tiles available", static_cast<int>(cached_atlas.subTextures.size()));
  }
  else {
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No texture/atlas");
  }
  ImGui::EndChild();

  ImGui::SameLine();

  // Splitter between Atlas and Properties panels
  ImGui::InvisibleButton("##Splitter", ImVec2(8.0f, panel_height));
  if (ImGui::IsItemActive()) {
    atlas_panel_width += ImGui::GetIO().MouseDelta.x;
  }
  if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
  }

  // Draw splitter visual
  ImVec2 splitter_min = ImGui::GetItemRectMin();
  ImVec2 splitter_max = ImGui::GetItemRectMax();
  ImDrawList* splitter_dl = ImGui::GetWindowDrawList();
  ImU32 splitter_col = ImGui::IsItemActive() ? IM_COL32(100, 150, 255, 255) :
    ImGui::IsItemHovered() ? IM_COL32(100, 100, 100, 255) : IM_COL32(60, 60, 60, 255);
  splitter_dl->AddRectFilled(ImVec2(splitter_min.x + 3, splitter_min.y), ImVec2(splitter_min.x + 5, splitter_max.y), splitter_col);

  ImGui::SameLine();

  // Right: Selected frame properties
  ImGui::BeginChild("PropertiesPanel", ImVec2(0, panel_height), true);

  // Animation Preview (when playing)
  if (is_previewing && !frame_indices.empty()) {
    ImGui::Text("Animation Preview");
    ImGui::Separator();

    // Calculate current frame based on elapsed time
    float elapsed = Engine::Time::get_time() - preview_start_time;
    float total_dur = 0.0f;
    for (float d : frame_durations) total_dur += d;

    int preview_frame_index = 0;
    if (total_dur > 0.0f) {
      float time_pos = is_looping ? fmod(elapsed, total_dur) : std::min(elapsed, total_dur);
      float accumulated = 0.0f;
      for (size_t i = 0; i < frame_durations.size(); i++) {
        accumulated += frame_durations[i];
        if (time_pos < accumulated) {
          preview_frame_index = static_cast<int>(i);
          break;
        }
        preview_frame_index = static_cast<int>(i);
      }
    }

    // Draw large animated preview
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 preview_pos = ImGui::GetCursorScreenPos();
    float preview_size = 150.0f;

    draw_frame_thumbnail(draw_list, preview_pos, ImVec2(preview_size, preview_size), frame_indices[preview_frame_index]);
    draw_list->AddRect(preview_pos, ImVec2(preview_pos.x + preview_size, preview_pos.y + preview_size),
      IM_COL32(255, 80, 80, 255), 0.0f, 0, 2.0f); // Red border when playing

    ImGui::Dummy(ImVec2(preview_size, preview_size));

    ImGui::Spacing();
    ImGui::Text("Frame %d / %d", preview_frame_index + 1, static_cast<int>(frame_indices.size()));
    ImGui::Text("Time: %.2fs / %.2fs", is_looping ? fmod(elapsed, total_dur) : std::min(elapsed, total_dur), total_dur);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
  }

  ImGui::Text("Frame Properties");
  ImGui::Separator();

  if (selected_frame_index >= 0 && selected_frame_index < static_cast<int>(frame_indices.size())) {
    // Large preview of selected frame
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 preview_pos = ImGui::GetCursorScreenPos();
    float preview_size = 120.0f;

    draw_frame_thumbnail(draw_list, preview_pos, ImVec2(preview_size, preview_size), frame_indices[selected_frame_index]);
    draw_list->AddRect(preview_pos, ImVec2(preview_pos.x + preview_size, preview_pos.y + preview_size),
      IM_COL32(100, 100, 100, 255));

    ImGui::Dummy(ImVec2(preview_size, preview_size));

    ImGui::Spacing();
    ImGui::Text("Frame %d of %d", selected_frame_index + 1, static_cast<int>(frame_indices.size()));
    ImGui::Text("Atlas Index: %d", frame_indices[selected_frame_index]);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Duration editor
    ImGui::Text("Duration:");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##duration", &frame_durations[selected_frame_index], 0.01f, 2.0f, "%.3f s")) {
      // Clamped by slider
    }

    ImGui::Spacing();

    // Quick duration buttons
    ImGui::Text("Quick Set:");
    if (ImGui::Button("0.05s", ImVec2(50, 0))) frame_durations[selected_frame_index] = 0.05f;
    ImGui::SameLine();
    if (ImGui::Button("0.1s", ImVec2(50, 0))) frame_durations[selected_frame_index] = 0.1f;
    ImGui::SameLine();
    if (ImGui::Button("0.15s", ImVec2(50, 0))) frame_durations[selected_frame_index] = 0.15f;
    ImGui::SameLine();
    if (ImGui::Button("0.2s", ImVec2(50, 0))) frame_durations[selected_frame_index] = 0.2f;

    if (ImGui::Button("0.25s", ImVec2(50, 0))) frame_durations[selected_frame_index] = 0.25f;
    ImGui::SameLine();
    if (ImGui::Button("0.5s", ImVec2(50, 0))) frame_durations[selected_frame_index] = 0.5f;
    ImGui::SameLine();
    if (ImGui::Button("1.0s", ImVec2(50, 0))) frame_durations[selected_frame_index] = 1.0f;

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Actions
    if (ImGui::Button("Duplicate Frame", ImVec2(-1, 0))) {
      int idx = frame_indices[selected_frame_index];
      float dur = frame_durations[selected_frame_index];
      frame_indices.insert(frame_indices.begin() + selected_frame_index + 1, idx);
      frame_durations.insert(frame_durations.begin() + selected_frame_index + 1, dur);
      selected_frame_index++;
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Delete Frame", ImVec2(-1, 0))) {
      frame_indices.erase(frame_indices.begin() + selected_frame_index);
      frame_durations.erase(frame_durations.begin() + selected_frame_index);
      if (selected_frame_index >= static_cast<int>(frame_indices.size())) {
        selected_frame_index = static_cast<int>(frame_indices.size()) - 1;
      }
    }
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Apply to all
    ImGui::Text("Apply to All Frames:");
    ImGui::SetNextItemWidth(100);
    ImGui::DragFloat("##default_dur", &default_duration, 0.01f, 0.01f, 2.0f, "%.3f s");
    ImGui::SameLine();
    if (ImGui::Button("Set All", ImVec2(-1, 0))) {
      for (float& d : frame_durations) d = default_duration;
    }

  }
  else {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Select a frame in the timeline\nto edit its properties");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Clear All Frames", ImVec2(-1, 0))) {
      frame_indices.clear();
      frame_durations.clear();
      selected_frame_index = -1;
    }
  }

  ImGui::EndChild();

  // ============================================================================
  // Bottom: Timeline
  // ============================================================================
  ImGui::Separator();
  draw_timeline();

  ImGui::End();

  if (!is_open) {
    FrameAnimationEditor::Close();
  }
}
