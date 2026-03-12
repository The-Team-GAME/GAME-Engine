//-----------------------------------------------------------------------------------
// file:    AnimatorTimelinePanel.cpp
// project: Monkey Stole My GPA
//
// brief:   Implementation of Animator Timeline panel for editing property animation
//          clips within state machine states. Provides a visual timeline with
//          keyframe editing, track management, easing selection, and playback preview.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "AnimatorTimelinePanel.h"
#include "../Precompiled.h"
#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Components/StateMachineComponents.h"
#include "../Components/AnimatorComponents.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include "../Time.h"
#include "../Logger.h"
#include <algorithm>
#include <map>
#include <set>
#include <sstream>

// Global wrapper functions for extern declarations in ReflectionFieldWidgets.cpp
namespace ImGuiWidgets {
  void AnimatorTimeline_Open(Engine::ECS::Entity entity, const std::string& state_name) {
    AnimatorTimeline::Open(entity, state_name);
  }
}

namespace {
  // ============================================================================
  // Panel State
  // ============================================================================
  s32 selected_entity_id = -1;
  std::string selected_state_name;
  bool selection_initialized = false;

  // Track/keyframe selection (multi-select)
  int selected_track_index = -1;         // Track list highlight
  std::set<std::pair<int, int>> selected_keyframes;  // {track_index, keyframe_index}
  int last_selected_track = -1;          // Last clicked keyframe's track (for inspector)
  int last_selected_keyframe = -1;       // Last clicked keyframe index (for inspector)

  // Copy/paste buffer
  struct CopiedKeyframe {
    int track_index;
    float relative_time;  // Offset from earliest copied keyframe
    double value;
  };
  std::vector<CopiedKeyframe> copy_buffer;

  // Timeline view state
  float timeline_zoom = 100.0f;   // Pixels per second
  float timeline_scroll_x = 0.0f;
  float track_height = 24.0f;

  // Dragging state
  bool is_dragging_keyframe = false;
  int drag_track_index = -1;
  int drag_keyframe_index = -1;

  // Scrubbing state (click/drag on ruler to move playhead)
  bool is_scrubbing = false;

  // Add Track state
  bool show_add_track_popup = false;
  std::string add_track_component_name;
  std::vector<std::pair<std::string, std::string>> add_track_available_fields; // (component_name, field_path)

  void init_selection_listener() {
    if (selection_initialized) return;
    selection_initialized = true;

    Engine::EventManager::add_event_listener<Engine::Events::SelectEntityEvent>(
      [](const Engine::Events::SelectEntityEvent& event) {
        s32 new_id = static_cast<s32>(event.entity_id);
        if (new_id == selected_entity_id) return;
        selected_entity_id = new_id;
        selected_state_name.clear();
        selected_track_index = -1;
        selected_keyframes.clear();
        last_selected_track = -1;
        last_selected_keyframe = -1;
        copy_buffer.clear();
        is_dragging_keyframe = false;
      }
    );
  }

  // ============================================================================
  // Helper: Enumerate numeric fields from a GenericValue (recursively)
  // ============================================================================
  void enumerate_numeric_fields(
    const ReflectionLibrary::GenericValue& gv,
    const std::string& prefix,
    std::vector<std::string>& out_paths)
  {
    if (gv.isDouble() || gv.isInt()) {
      out_paths.push_back(prefix);
    }
    else if (gv.isObject()) {
      const auto& obj = gv.getObject();
      for (const auto& [key, val] : obj) {
        // Skip type_hint and non-numeric nested objects that are complex types
        if (key == "type_hint") continue;
        std::string path = prefix.empty() ? key : prefix + "." + key;
        enumerate_numeric_fields(val, path, out_paths);
      }
    }
  }

  // ============================================================================
  // Helper: Get PropertyAnimationClip from the StateMachine for current state
  // ============================================================================
  Components::StateMachine* get_state_machine() {
    if (selected_entity_id < 0) return nullptr;
    Engine::ECS::Entity entity(static_cast<Engine::ECS::EntityID>(selected_entity_id));
    if (!entity.is_valid()) return nullptr;
    return entity.get_component<Components::StateMachine>();
  }

  Components::PropertyAnimationClip* get_current_clip() {
    auto* sm = get_state_machine();
    if (!sm) return nullptr;

    auto it = sm->states.find(selected_state_name);
    if (it == sm->states.end()) return nullptr;
    if (!it->second.property_animation.has_value()) return nullptr;

    return &it->second.property_animation.value();
  }

  // ============================================================================
  // Helper: Time to pixel X position in timeline
  // ============================================================================
  float time_to_px(float time_sec, float origin_x) {
    return origin_x + (time_sec * timeline_zoom) - timeline_scroll_x;
  }

  // ============================================================================
  // Helper: Pixel X position to time
  // ============================================================================
  float px_to_time(float px, float origin_x) {
    return (px - origin_x + timeline_scroll_x) / timeline_zoom;
  }

  // ============================================================================  
  // Render: Track list (left pane) 
  // ============================================================================
  void render_track_list(Components::PropertyAnimationClip* clip) {
    if (!clip) return;

    for (int i = 0; i < static_cast<int>(clip->tracks.size()); ++i) {
      auto& track = clip->tracks[i];

      bool is_selected = (i == selected_track_index);
      std::string label = track.target_component + " > " + track.target_field_path;

      ImGui::PushID(i);
      if (ImGui::Selectable(label.c_str(), is_selected, 0, ImVec2(0, track_height))) {
        selected_track_index = i;
        selected_keyframes.clear();
        last_selected_track = -1;
        last_selected_keyframe = -1;
      }

      // Right-click context menu
      if (ImGui::BeginPopupContextItem("track_ctx")) {
        // Easing selector submenu
        if (ImGui::BeginMenu("Easing")) {
          for (int e = 0; e <= static_cast<int>(Components::EasingType::ElasticInOut); ++e) {
            auto etype = static_cast<Components::EasingType>(e);
            bool is_current = (track.easing == etype);
            if (ImGui::MenuItem(Components::easing_type_name(etype), nullptr, is_current)) {
              track.easing = etype;
            }
          }
          ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Remove Track")) {
          clip->tracks.erase(clip->tracks.begin() + i);
          // Update multi-selection: remove deleted track, adjust indices
          std::set<std::pair<int, int>> new_sel;
          for (const auto& sel : selected_keyframes) {
            if (sel.first == i) continue;
            int new_trk = sel.first > i ? sel.first - 1 : sel.first;
            new_sel.insert({ new_trk, sel.second });
          }
          selected_keyframes = std::move(new_sel);
          if (last_selected_track == i) { last_selected_track = -1; last_selected_keyframe = -1; }
          else if (last_selected_track > i) { last_selected_track--; }
          if (selected_track_index == i) {
            selected_track_index = -1;
          }
          else if (selected_track_index > i) {
            selected_track_index--;
          }
          ImGui::EndPopup();
          ImGui::PopID();
          return; // List modified, exit to avoid stale iterators
        }
        ImGui::EndPopup();
      }
      ImGui::PopID();
    }
  }

  // ============================================================================  
  // Render: Timeline area (right pane)
  // ============================================================================
  void render_timeline(Components::PropertyAnimationClip* clip, float origin_x, float origin_y, float width, float height) {
    if (!clip) return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Colors
    const ImU32 col_bg = IM_COL32(30, 30, 30, 255);
    const ImU32 col_ruler = IM_COL32(80, 80, 80, 255);
    const ImU32 col_ruler_text = IM_COL32(180, 180, 180, 255);
    const ImU32 col_track_bg = IM_COL32(40, 40, 40, 255);
    const ImU32 col_track_alt = IM_COL32(45, 45, 50, 255);
    const ImU32 col_keyframe = IM_COL32(255, 200, 50, 255);
    const ImU32 col_keyframe_selected = IM_COL32(100, 200, 255, 255);
    const ImU32 col_playhead = IM_COL32(255, 80, 80, 255);
    const ImU32 col_duration_line = IM_COL32(100, 100, 100, 200);

    float ruler_height = 20.0f;

    // Background
    draw_list->AddRectFilled(
      ImVec2(origin_x, origin_y),
      ImVec2(origin_x + width, origin_y + height),
      col_bg);

    // Draw ruler
    {
      float time_start = timeline_scroll_x / timeline_zoom;
      float time_end = time_start + width / timeline_zoom;

      // Determine tick spacing based on zoom
      float tick_interval = 0.1f;
      if (timeline_zoom < 50.0f) tick_interval = 1.0f;
      else if (timeline_zoom < 150.0f) tick_interval = 0.5f;
      else if (timeline_zoom < 300.0f) tick_interval = 0.25f;

      float first_tick = std::floor(time_start / tick_interval) * tick_interval;
      for (float t = first_tick; t <= time_end; t += tick_interval) {
        float px = time_to_px(t, origin_x);
        if (px < origin_x || px > origin_x + width) continue;

        bool is_major = std::fmod(t, tick_interval * 2.0f) < 0.001f || tick_interval >= 0.5f;
        float line_height = is_major ? ruler_height : ruler_height * 0.5f;
        draw_list->AddLine(
          ImVec2(px, origin_y + ruler_height - line_height),
          ImVec2(px, origin_y + ruler_height),
          col_ruler, 1.0f);

        if (is_major) {
          char buf[16];
          snprintf(buf, sizeof(buf), "%.2fs", t);
          draw_list->AddText(ImVec2(px + 2.0f, origin_y), col_ruler_text, buf);
        }
      }
    }

    // Duration end marker
    {
      float dur_px = time_to_px(clip->duration, origin_x);
      draw_list->AddLine(
        ImVec2(dur_px, origin_y),
        ImVec2(dur_px, origin_y + height),
        col_duration_line, 2.0f);
    }

    // Draw track lanes and keyframes
    bool keyframes_modified = false;
    float track_y = origin_y + ruler_height;
    for (int ti = 0; ti < static_cast<int>(clip->tracks.size()); ++ti) {
      auto& track = clip->tracks[ti];
      ImU32 bg_col = (ti % 2 == 0) ? col_track_bg : col_track_alt;

      // Track background
      draw_list->AddRectFilled(
        ImVec2(origin_x, track_y),
        ImVec2(origin_x + width, track_y + track_height),
        bg_col);

      // Draw keyframe diamonds
      for (int ki = 0; ki < static_cast<int>(track.keyframes.size()); ++ki) {
        auto& kf = track.keyframes[ki];
        float kf_px = time_to_px(kf.time, origin_x);
        float kf_cy = track_y + track_height * 0.5f;
        float diamond_size = 5.0f;

        bool is_sel = selected_keyframes.count({ ti, ki }) > 0;
        ImU32 kf_col = is_sel ? col_keyframe_selected : col_keyframe;

        // Draw diamond
        draw_list->AddQuadFilled(
          ImVec2(kf_px, kf_cy - diamond_size),
          ImVec2(kf_px + diamond_size, kf_cy),
          ImVec2(kf_px, kf_cy + diamond_size),
          ImVec2(kf_px - diamond_size, kf_cy),
          kf_col);

        // Hit test for clicking/dragging keyframes
        ImVec2 kf_min(kf_px - diamond_size - 2, track_y);
        ImVec2 kf_max(kf_px + diamond_size + 2, track_y + track_height);

        ImGui::SetCursorScreenPos(kf_min);
        std::string kf_id = "##kf_" + std::to_string(ti) + "_" + std::to_string(ki);
        ImGui::InvisibleButton(kf_id.c_str(), ImVec2(kf_max.x - kf_min.x, kf_max.y - kf_min.y));

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
          bool ctrl = ImGui::GetIO().KeyCtrl;
          if (ctrl) {
            // Toggle in multi-selection
            auto key = std::make_pair(ti, ki);
            if (selected_keyframes.count(key))
              selected_keyframes.erase(key);
            else
              selected_keyframes.insert(key);
          }
          else {
            // If clicking an already-selected keyframe, keep multi-select (for dragging)
            if (selected_keyframes.count({ ti, ki }) == 0)
              selected_keyframes.clear();
            selected_keyframes.insert({ ti, ki });
          }
          selected_track_index = ti;
          last_selected_track = ti;
          last_selected_keyframe = ki;
          is_dragging_keyframe = true;
          drag_track_index = ti;
          drag_keyframe_index = ki;
        }

        // Right-click context menu
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
          selected_track_index = ti;
          last_selected_track = ti;
          last_selected_keyframe = ki;
          // If right-clicked keyframe isn't in selection, select only it
          if (selected_keyframes.count({ ti, ki }) == 0) {
            selected_keyframes.clear();
            selected_keyframes.insert({ ti, ki });
          }
        }
        if (ImGui::BeginPopupContextItem(("kf_ctx_" + std::to_string(ti) + "_" + std::to_string(ki)).c_str())) {
          if (ImGui::MenuItem("Delete Selected Keyframes")) {
            // Delete all selected keyframes (reverse order to maintain valid indices)
            for (auto sit = selected_keyframes.rbegin(); sit != selected_keyframes.rend(); ++sit) {
              int trk = sit->first;
              int idx = sit->second;
              if (trk >= 0 && trk < static_cast<int>(clip->tracks.size()) &&
                idx >= 0 && idx < static_cast<int>(clip->tracks[trk].keyframes.size())) {
                clip->tracks[trk].keyframes.erase(clip->tracks[trk].keyframes.begin() + idx);
              }
            }
            selected_keyframes.clear();
            last_selected_track = -1;
            last_selected_keyframe = -1;
            keyframes_modified = true;
            ImGui::EndPopup();
            break;
          }
          ImGui::EndPopup();
        }
      }

      if (keyframes_modified) break;

      // Click/double-click on empty track area
      {
        ImGui::SetCursorScreenPos(ImVec2(origin_x, track_y));
        std::string track_btn_id = "##track_area_" + std::to_string(ti);
        ImGui::InvisibleButton(track_btn_id.c_str(), ImVec2(width, track_height));

        // Single click on empty area deselects all keyframes
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().KeyCtrl) {
          selected_keyframes.clear();
          last_selected_track = -1;
          last_selected_keyframe = -1;
        }

        // Double-click on empty area to add keyframe
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          float click_time = px_to_time(ImGui::GetMousePos().x, origin_x);
          click_time = std::max(0.0f, std::min(click_time, clip->duration));

          // Sample current value from entity
          double sample_val = track.evaluate(click_time);

          Components::Keyframe new_kf(click_time, sample_val);
          track.keyframes.push_back(new_kf);
          track.sort_keyframes();

          // Find and select the newly added keyframe
          for (int ki = 0; ki < static_cast<int>(track.keyframes.size()); ++ki) {
            if (std::abs(track.keyframes[ki].time - click_time) < 0.001f) {
              selected_track_index = ti;
              selected_keyframes.clear();
              selected_keyframes.insert({ ti, ki });
              last_selected_track = ti;
              last_selected_keyframe = ki;
              break;
            }
          }
        }
      }

      track_y += track_height;
    }

    // Handle keyframe dragging (multi-select: moves all selected keyframes)
    if (is_dragging_keyframe && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      float mouse_delta_x = ImGui::GetIO().MouseDelta.x;
      float time_delta = mouse_delta_x / timeline_zoom;
      if (std::abs(time_delta) > 0.0f) {
        for (const auto& sel : selected_keyframes) {
          int trk = sel.first;
          int idx = sel.second;
          if (trk >= 0 && trk < static_cast<int>(clip->tracks.size()) &&
            idx >= 0 && idx < static_cast<int>(clip->tracks[trk].keyframes.size())) {
            float new_time = clip->tracks[trk].keyframes[idx].time + time_delta;
            new_time = std::max(0.0f, std::min(new_time, clip->duration));
            clip->tracks[trk].keyframes[idx].time = new_time;
          }
        }
      }
    }
    if (is_dragging_keyframe && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      // Record times of selected keyframes before sorting
      std::map<int, std::vector<float>> sel_times;
      for (const auto& sel : selected_keyframes) {
        int trk = sel.first;
        int idx = sel.second;
        if (trk >= 0 && trk < static_cast<int>(clip->tracks.size()) &&
          idx >= 0 && idx < static_cast<int>(clip->tracks[trk].keyframes.size())) {
          sel_times[trk].push_back(clip->tracks[trk].keyframes[idx].time);
        }
      }
      // Sort affected tracks
      std::set<int> affected_tracks;
      for (const auto& sel : selected_keyframes) affected_tracks.insert(sel.first);
      for (int trk : affected_tracks) {
        if (trk >= 0 && trk < static_cast<int>(clip->tracks.size()))
          clip->tracks[trk].sort_keyframes();
      }
      // Rebuild selection with updated indices
      selected_keyframes.clear();
      for (const auto& [trk, times] : sel_times) {
        if (trk >= 0 && trk < static_cast<int>(clip->tracks.size())) {
          for (float t : times) {
            for (int ki = 0; ki < static_cast<int>(clip->tracks[trk].keyframes.size()); ++ki) {
              if (std::abs(clip->tracks[trk].keyframes[ki].time - t) < 0.001f) {
                selected_keyframes.insert({ trk, ki });
                break;
              }
            }
          }
        }
      }
      is_dragging_keyframe = false;
      drag_track_index = -1;
      drag_keyframe_index = -1;
    }

    // Ruler click/drag to move playhead
    {
      ImGui::SetCursorScreenPos(ImVec2(origin_x, origin_y));
      ImGui::InvisibleButton("##ruler_hit", ImVec2(width, ruler_height));

      if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        is_scrubbing = true;
        float click_time = px_to_time(ImGui::GetMousePos().x, origin_x);
        if (auto* sm = get_state_machine()) sm->set_property_anim_elapsed(std::max(0.0f, std::min(click_time, clip->duration)));
      }
    }
    if (is_scrubbing && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      float drag_time = px_to_time(ImGui::GetMousePos().x, origin_x);
      if (auto* sm = get_state_machine()) sm->set_property_anim_elapsed(std::max(0.0f, std::min(drag_time, clip->duration)));
    }
    if (is_scrubbing && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      is_scrubbing = false;
    }

    // Keyboard shortcuts (Ctrl+C, Ctrl+V, Delete, Ctrl+A)
    if (ImGui::IsWindowFocused()) {
      ImGuiIO& io = ImGui::GetIO();

      // Delete selected keyframes
      if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !selected_keyframes.empty()) {
        for (auto it = selected_keyframes.rbegin(); it != selected_keyframes.rend(); ++it) {
          int trk = it->first;
          int idx = it->second;
          if (trk >= 0 && trk < static_cast<int>(clip->tracks.size()) &&
            idx >= 0 && idx < static_cast<int>(clip->tracks[trk].keyframes.size())) {
            clip->tracks[trk].keyframes.erase(clip->tracks[trk].keyframes.begin() + idx);
          }
        }
        selected_keyframes.clear();
        last_selected_track = -1;
        last_selected_keyframe = -1;
      }

      // Ctrl+A: Select all keyframes
      if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
        selected_keyframes.clear();
        for (int ti = 0; ti < static_cast<int>(clip->tracks.size()); ++ti) {
          for (int ki = 0; ki < static_cast<int>(clip->tracks[ti].keyframes.size()); ++ki) {
            selected_keyframes.insert({ ti, ki });
          }
        }
      }

      // Ctrl+C: Copy selected keyframes
      if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C) && !selected_keyframes.empty()) {
        copy_buffer.clear();
        float earliest_time = 1e30f;
        for (const auto& sel : selected_keyframes) {
          int trk = sel.first;
          int idx = sel.second;
          if (trk >= 0 && trk < static_cast<int>(clip->tracks.size()) &&
            idx >= 0 && idx < static_cast<int>(clip->tracks[trk].keyframes.size())) {
            float t = clip->tracks[trk].keyframes[idx].time;
            if (t < earliest_time) earliest_time = t;
          }
        }
        for (const auto& sel : selected_keyframes) {
          int trk = sel.first;
          int idx = sel.second;
          if (trk >= 0 && trk < static_cast<int>(clip->tracks.size()) &&
            idx >= 0 && idx < static_cast<int>(clip->tracks[trk].keyframes.size())) {
            auto& kf = clip->tracks[trk].keyframes[idx];
            copy_buffer.push_back({ trk, kf.time - earliest_time, kf.value });
          }
        }
      }

      // Ctrl+V: Paste keyframes at playhead position
      if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V) && !copy_buffer.empty()) {
        selected_keyframes.clear();
        for (const auto& ckf : copy_buffer) {
          int trk = ckf.track_index;
          if (trk >= 0 && trk < static_cast<int>(clip->tracks.size())) {
            float paste_time = (get_state_machine() ? get_state_machine()->get_property_anim_elapsed() : 0.0f) + ckf.relative_time;
            paste_time = std::max(0.0f, std::min(paste_time, clip->duration));
            clip->tracks[trk].keyframes.push_back(Components::Keyframe(paste_time, ckf.value));
            clip->tracks[trk].sort_keyframes();
            // Select the pasted keyframe
            for (int ki = 0; ki < static_cast<int>(clip->tracks[trk].keyframes.size()); ++ki) {
              if (std::abs(clip->tracks[trk].keyframes[ki].time - paste_time) < 0.001f) {
                selected_keyframes.insert({ trk, ki });
                break;
              }
            }
          }
        }
      }
    }

    // Draw playhead
    {
      float ph_px = time_to_px(get_state_machine() ? get_state_machine()->get_property_anim_elapsed() : 0.0f, origin_x);
      draw_list->AddLine(
        ImVec2(ph_px, origin_y),
        ImVec2(ph_px, origin_y + height),
        col_playhead, 2.0f);

      // Triangle at top
      draw_list->AddTriangleFilled(
        ImVec2(ph_px - 5, origin_y),
        ImVec2(ph_px + 5, origin_y),
        ImVec2(ph_px, origin_y + 8),
        col_playhead);
    }

    // Mouse wheel zoom
    if (ImGui::IsWindowHovered()) {
      float wheel = ImGui::GetIO().MouseWheel;
      if (wheel != 0.0f) {
        float old_zoom = timeline_zoom;
        timeline_zoom *= (1.0f + wheel * 0.1f);
        timeline_zoom = std::max(20.0f, std::min(timeline_zoom, 1000.0f));

        // Zoom towards mouse position
        float mouse_x = ImGui::GetMousePos().x;
        float time_at_mouse = (mouse_x - origin_x + timeline_scroll_x) / old_zoom;
        timeline_scroll_x = time_at_mouse * timeline_zoom - (mouse_x - origin_x);
        timeline_scroll_x = std::max(0.0f, timeline_scroll_x);
      }
    }

    // Middle mouse button pan
    if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
      timeline_scroll_x -= ImGui::GetIO().MouseDelta.x;
      timeline_scroll_x = std::max(0.0f, timeline_scroll_x);
    }
  }

  // ============================================================================  
  // Render: Keyframe inspector (bottom pane)
  // ============================================================================
  void render_keyframe_inspector(Components::PropertyAnimationClip* clip) {
    if (!clip) return;

    // Show selection count
    if (!selected_keyframes.empty()) {
      ImGui::Text("Selected: %d keyframe(s)", static_cast<int>(selected_keyframes.size()));
      ImGui::SameLine(0, 15);
    }

    if (last_selected_track < 0 || last_selected_track >= static_cast<int>(clip->tracks.size())) {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select a keyframe to inspect");
      return;
    }

    auto& track = clip->tracks[last_selected_track];
    ImGui::Text("Track: %s > %s", track.target_component.c_str(), track.target_field_path.c_str());

    // Easing selector for track
    ImGui::SameLine(0, 20);
    ImGui::Text("Easing:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    if (ImGui::BeginCombo("##easing_select", Components::easing_type_name(track.easing))) {
      for (int e = 0; e <= static_cast<int>(Components::EasingType::ElasticInOut); ++e) {
        auto etype = static_cast<Components::EasingType>(e);
        bool is_current = (track.easing == etype);
        if (ImGui::Selectable(Components::easing_type_name(etype), is_current)) {
          track.easing = etype;
        }
      }
      ImGui::EndCombo();
    }

    if (last_selected_keyframe < 0 || last_selected_keyframe >= static_cast<int>(track.keyframes.size())) {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No keyframe selected");
      return;
    }

    auto& kf = track.keyframes[last_selected_keyframe];
    ImGui::Separator();

    ImGui::Text("Keyframe %d / %d", last_selected_keyframe + 1, static_cast<int>(track.keyframes.size()));

    // Time editor
    ImGui::SetNextItemWidth(120);
    float kf_time = kf.time;
    if (ImGui::DragFloat("Time (s)##kf_time", &kf_time, 0.01f, 0.0f, clip->duration, "%.3f")) {
      kf.time = kf_time;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      track.sort_keyframes();
      // Reset selection as indices may have changed
      last_selected_keyframe = -1;
      selected_keyframes.clear();
    }

    // Value editor
    ImGui::SameLine(0, 20);
    ImGui::SetNextItemWidth(120);
    float kf_val = static_cast<float>(kf.value);
    if (ImGui::DragFloat("Value##kf_val", &kf_val, 0.01f, -10000.0f, 10000.0f, "%.4f")) {
      kf.value = static_cast<double>(kf_val);
    }
  }

  // ============================================================================  
  // Populate available fields for a given entity
  // ============================================================================
  void populate_available_fields(Engine::ECS::Entity entity) {
    add_track_available_fields.clear();

    entity.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray* array) {
      if (!array->is_reflectable()) return;
      // Skip StateMachine itself and non-animatable types
      if (type_name == "Components::StateMachine") return;

      auto gv = array->to_generic_value(entity.get_id());
      if (!gv.isObject()) return;

      std::vector<std::string> paths;
      enumerate_numeric_fields(gv, "", paths);

      for (const auto& path : paths) {
        add_track_available_fields.emplace_back(type_name, path);
      }
      });
  }

} // anonymous namespace

// ============================================================================
// Public API
// ============================================================================

namespace AnimatorTimeline {

  void Open(Engine::ECS::Entity entity, const std::string& state_name) {
    selected_entity_id = static_cast<s32>(entity.get_id());
    selected_state_name = state_name;
    selected_track_index = -1;
    selected_keyframes.clear();
    last_selected_track = -1;
    last_selected_keyframe = -1;
    copy_buffer.clear();
    is_dragging_keyframe = false;

    // Stop any ongoing property animation preview on the state machine
    auto* sm = entity.get_component<Components::StateMachine>();
    if (sm) sm->stop_property_anim();

    // Populate available fields
    populate_available_fields(entity);
  }

} // namespace AnimatorTimeline

// ============================================================================
// Main Panel Draw Function
// ============================================================================

void AnimatorTimelinePanel() {
  init_selection_listener();

  ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Animator Timeline")) {
    ImGui::End();
    return;
  }

  // Validate entity
  if (selected_entity_id < 0) {
    ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No entity selected");
    ImGui::End();
    return;
  }

  Engine::ECS::Entity entity(static_cast<Engine::ECS::EntityID>(selected_entity_id));
  if (!entity.is_valid()) {
    ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "Entity is no longer valid");
    ImGui::End();
    return;
  }

  auto* sm = entity.get_component<Components::StateMachine>();
  if (!sm) {
    ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "Selected entity has no StateMachine");
    ImGui::End();
    return;
  }

  // Auto-pick state: use the current state of the state machine
  if (selected_state_name.empty() || sm->states.find(selected_state_name) == sm->states.end()) {
    selected_state_name = sm->get_current_state();
  }

  // State selector combo
  if (ImGui::BeginCombo("State##anim_state_select", selected_state_name.c_str())) {
    for (const auto& [name, state] : sm->states) {
      bool is_selected = (name == selected_state_name);
      if (ImGui::Selectable(name.c_str(), is_selected)) {
        selected_state_name = name;
        selected_track_index = -1;
        selected_keyframes.clear();
        last_selected_track = -1;
        last_selected_keyframe = -1;
      }
      if (is_selected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if (selected_state_name.empty()) {
    ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No state available");
    ImGui::End();
    return;
  }

  auto state_it = sm->states.find(selected_state_name);
  if (state_it == sm->states.end()) {
    ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "State '%s' no longer exists", selected_state_name.c_str());
    ImGui::End();
    return;
  }

  auto& state = state_it->second;
  if (!state.property_animation.has_value()) {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "State '%s' has no property animation clip.", selected_state_name.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Add Property Animation")) {
      state.property_animation = Components::PropertyAnimationClip();
    }
    ImGui::End();
    return;
  }

  Components::PropertyAnimationClip* clip = &state.property_animation.value();

  // ========================
  // Header bar
  // ========================
  ImGui::SetNextItemWidth(100);
  ImGui::DragFloat("Duration##clip_dur", &clip->duration, 0.05f, 0.1f, 60.0f, "%.2fs");
  ImGui::SameLine();
  ImGui::Checkbox("Loop##clip_loop", &clip->is_looping);

  // Playback controls
  ImGui::SameLine(0, 20);
  if (!sm->is_property_anim_playing()) {
    if (ImGui::Button("Play")) {
      sm->set_property_anim_elapsed(0.0f);
      sm->play_property_anim();
    }
  }
  else {
    if (ImGui::Button("Pause")) {
      sm->pause_property_anim();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Stop")) {
    sm->stop_property_anim();
  }

  // Add Track button
  ImGui::SameLine(0, 20);
  if (ImGui::Button("+ Add Track")) {
    populate_available_fields(entity);
    ImGui::OpenPopup("AddTrackPopup");
  }

  // Add Track popup
  if (ImGui::BeginPopup("AddTrackPopup")) {
    ImGui::Text("Select property to animate:");
    ImGui::Separator();

    // Group by component
    std::string current_component;
    for (const auto& [comp_name, field_path] : add_track_available_fields) {
      // Check if this track already exists
      bool already_exists = false;
      for (const auto& existing_track : clip->tracks) {
        if (existing_track.target_component == comp_name && existing_track.target_field_path == field_path) {
          already_exists = true;
          break;
        }
      }

      if (comp_name != current_component) {
        current_component = comp_name;
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", comp_name.c_str());
      }

      std::string label = "  " + field_path;
      if (already_exists) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s (exists)", label.c_str());
      }
      else {
        if (ImGui::MenuItem(label.c_str())) {
          Components::AnimationTrack new_track;
          new_track.target_component = comp_name;
          new_track.target_field_path = field_path;

          // Sample current value and create 2 default keyframes
          double current_val = 0.0;
          // Try to read current value from entity
          entity.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray* array) {
            if (type_name != comp_name) return;
            if (!array->is_reflectable()) return;
            auto gv = array->to_generic_value(entity.get_id());

            // Navigate path to get current value
            ReflectionLibrary::GenericValue* cur = &gv;
            std::string seg;
            std::istringstream ss(field_path);
            while (std::getline(ss, seg, '.')) {
              if (cur->isObject()) {
                auto& obj = cur->getObject();
                auto it = obj.find(seg);
                if (it != obj.end()) {
                  cur = &it->second;
                }
                else break;
              }
              else break;
            }
            if (cur->isDouble()) current_val = cur->getDouble();
            else if (cur->isInt()) current_val = static_cast<double>(cur->getInt());
            });

          new_track.keyframes.push_back(Components::Keyframe(0.0f, current_val));
          new_track.keyframes.push_back(Components::Keyframe(clip->duration, current_val));
          clip->tracks.push_back(std::move(new_track));

          selected_track_index = static_cast<int>(clip->tracks.size()) - 1;
          selected_keyframes.clear();
          last_selected_track = -1;
          last_selected_keyframe = -1;
          ImGui::CloseCurrentPopup();
        }
      }
    }
    ImGui::EndPopup();
  }

  ImGui::Separator();

  // ========================
  // Main content area: Track list (left) + Timeline (right)
  // ========================
  float left_pane_width = 200.0f;
  float content_height = ImGui::GetContentRegionAvail().y - 80.0f; // Reserve space for inspector
  content_height = std::max(content_height, 100.0f);

  // Left pane: Track list
  ImGui::BeginChild("TrackList", ImVec2(left_pane_width, content_height), true);
  {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Tracks (%d)", static_cast<int>(clip->tracks.size()));
    ImGui::Separator();
    render_track_list(clip);
  }
  ImGui::EndChild();

  ImGui::SameLine();

  // Right pane: Timeline canvas
  ImGui::BeginChild("Timeline", ImVec2(0, content_height), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
  {
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();

    render_timeline(clip, canvas_pos.x, canvas_pos.y, canvas_size.x, canvas_size.y);

    // Advance preview playback
    if (sm->is_property_anim_playing()) {
      float elapsed = sm->get_property_anim_elapsed() + Engine::Time::get_real_dt();
      if (elapsed >= clip->duration) {
        if (clip->is_looping) {
          elapsed = std::fmod(elapsed, clip->duration);
        }
        else {
          elapsed = clip->duration;
          sm->pause_property_anim();
        }
      }
      sm->set_property_anim_elapsed(elapsed);
    }

    // Apply animation preview to entity (during playback or scrubbing)
    if (sm->is_property_anim_playing() || is_scrubbing) {
      for (const auto& track : clip->tracks) {
        double val = track.evaluate(sm->get_property_anim_elapsed());

        entity.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray* array) {
          if (type_name != track.target_component) return;
          if (!array->is_reflectable()) return;

          auto gv = array->to_generic_value(entity.get_id());
          if (Components::StateMachine::set_generic_value_at_path(gv, track.target_field_path, val)) {
            array->set_component_from_generic_value(entity.get_id(), gv);
          }
          });
      }
    }
  }
  ImGui::EndChild();

  // ========================
  // Bottom pane: Keyframe inspector
  // ========================
  ImGui::Separator();
  ImGui::BeginChild("KeyframeInspector", ImVec2(0, 0), true);
  render_keyframe_inspector(clip);
  ImGui::EndChild();

  ImGui::End();
}
