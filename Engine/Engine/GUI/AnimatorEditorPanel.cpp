//-----------------------------------------------------------------------------------
// file:    AnimatorEditorPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the AnimatorEditorPanel ImGui interface for
//          visually editing Animator components with multiple tracks, keyframes,
//          timeline visualization, and easing function selection.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "AnimatorEditorPanel.h"
#include "../Precompiled.h"
#include "../ECS/ECS.h"
#include "../Components/AnimatorComponents.h"
#include "../Components/Components.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include "../FunctionRegistry.h"
#include "../Easing.h"
#include "../Time.h"
#include "../Logger.h"

namespace {
  enum class EditorMode { None, Entity, Prefab };

  EditorMode current_mode = EditorMode::None;
  s32 selected_entity_id = -1;
  Engine::ECS::Prefab selected_prefab;
  std::string selected_prefab_path = "";

  bool selection_initialized = false;
  int selected_track_index = -1;
  int selected_keyframe_index = -1;

  // Available easing function names
  const std::vector<std::string> easing_names = {
    "EaseLinearNone",
    "EaseSineIn", "EaseSineOut", "EaseSineInOut",
    "EaseQuadIn", "EaseQuadOut", "EaseQuadInOut",
    "EaseCubicIn", "EaseCubicOut", "EaseCubicInOut",
    "EaseCircIn", "EaseCircOut", "EaseCircInOut",
    "EaseExpoIn", "EaseExpoOut", "EaseExpoInOut",
    "EaseBackIn", "EaseBackOut", "EaseBackInOut",
    "EaseBounceIn", "EaseBounceOut", "EaseBounceInOut",
    "EaseElasticIn", "EaseElasticOut", "EaseElasticInOut"
  };

  // Common component types for dropdown
  const std::vector<std::string> common_component_types = {
    "Components::Transform2D",
    "Components::Color",
    "Components::Distortion"
  };

  // Common field paths for each component type
  const std::map<std::string, std::vector<std::string>> common_field_paths = {
    {"Components::Transform2D", {"position.x", "position.y", "rotation", "scale.x", "scale.y"}},
    {"Components::Color", {"r", "g", "b", "a"}},
    {"Components::Distortion", {"intensity", "speed", "mesh_intensity"}}
  };

  void init_selection_listener() {
    if (selection_initialized) return;
    selection_initialized = true;

    Engine::EventManager::add_event_listener<Engine::Events::SelectEntityEvent>(
      [](const Engine::Events::SelectEntityEvent& event) {
        if (event.entity_id > 0) {
          selected_entity_id = static_cast<s32>(event.entity_id);
          current_mode = EditorMode::Entity;
          selected_track_index = -1;
          selected_keyframe_index = -1;
        }
      }
    );
  }

  // Helper to render easing dropdown
  bool EasingCombo(const char* label, std::string& current_easing) {
    int current_index = 0;
    for (size_t i = 0; i < easing_names.size(); ++i) {
      if (easing_names[i] == current_easing) {
        current_index = static_cast<int>(i);
        break;
      }
    }

    bool changed = false;
    if (ImGui::BeginCombo(label, current_easing.c_str())) {
      for (size_t i = 0; i < easing_names.size(); ++i) {
        bool is_selected = (current_easing == easing_names[i]);
        if (ImGui::Selectable(easing_names[i].c_str(), is_selected)) {
          current_easing = easing_names[i];
          changed = true;
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    return changed;
  }

  // Helper to draw easing curve preview
  void DrawEasingCurvePreview(ImDrawList* draw_list, ImVec2 pos, ImVec2 size, const std::string& easing_name) {
    using EasingFunc = f32(f32, f32, f32, f32);
    auto easing_func = Engine::FunctionRegistry<EasingFunc>::get_function(easing_name);
    if (!easing_func) {
      easing_func = Engine::Easing::EaseLinearNone;
    }

    // Draw background
    draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(40, 40, 40, 255));
    draw_list->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(80, 80, 80, 255));

    // Draw curve
    const int segments = 32;
    ImVec2 prev_point = pos;
    prev_point.y = pos.y + size.y;

    for (int i = 1; i <= segments; ++i) {
      float t = static_cast<float>(i) / segments;
      float value = easing_func(t, 0.0f, 1.0f, 1.0f);

      ImVec2 point;
      point.x = pos.x + t * size.x;
      point.y = pos.y + size.y - value * size.y;

      draw_list->AddLine(prev_point, point, IM_COL32(100, 200, 255, 255), 2.0f);
      prev_point = point;
    }
  }

  // Track colors for visual distinction
  ImU32 GetTrackColor(int track_index) {
    const ImU32 colors[] = {
      IM_COL32(255, 100, 100, 255),  // Red
      IM_COL32(100, 255, 100, 255),  // Green
      IM_COL32(100, 100, 255, 255),  // Blue
      IM_COL32(255, 255, 100, 255),  // Yellow
      IM_COL32(255, 100, 255, 255),  // Magenta
      IM_COL32(100, 255, 255, 255),  // Cyan
      IM_COL32(255, 180, 100, 255),  // Orange
      IM_COL32(180, 100, 255, 255),  // Purple
    };
    return colors[track_index % 8];
  }

  // Draw timeline with all tracks and keyframes
  void DrawTimeline(ImDrawList* draw_list, ImVec2 pos, ImVec2 size,
    Components::Animator* anim, float preview_normalized_time) {
    // Background
    draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(30, 30, 35, 255));
    draw_list->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(80, 80, 80, 255));

    // Draw time markers
    for (int i = 0; i <= 10; ++i) {
      float t = i / 10.0f;
      float x = pos.x + t * size.x;
      draw_list->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + 5), IM_COL32(100, 100, 100, 255));

      if (i % 2 == 0) {
        char label[16];
        snprintf(label, sizeof(label), "%.1f", t);
        draw_list->AddText(ImVec2(x - 8, pos.y + 6), IM_COL32(150, 150, 150, 255), label);
      }
    }

    // Draw tracks
    float track_height = 20.0f;
    float tracks_start_y = pos.y + 25.0f;

    for (size_t track_idx = 0; track_idx < anim->tracks.size(); ++track_idx) {
      const auto& track = anim->tracks[track_idx];
      float track_y = tracks_start_y + track_idx * track_height;
      ImU32 track_color = GetTrackColor(static_cast<int>(track_idx));

      // Track background
      bool is_selected_track = (selected_track_index == static_cast<int>(track_idx));
      ImU32 bg_color = is_selected_track ? IM_COL32(50, 50, 60, 255) : IM_COL32(35, 35, 40, 255);
      draw_list->AddRectFilled(
        ImVec2(pos.x, track_y),
        ImVec2(pos.x + size.x, track_y + track_height - 2),
        bg_color
      );

      // Track label (abbreviated)
      std::string label = track.field_path;
      if (label.length() > 12) {
        label = label.substr(0, 10) + "..";
      }
      draw_list->AddText(ImVec2(pos.x + 2, track_y + 2), track_color, label.c_str());

      // Draw keyframes for this track
      for (size_t kf_idx = 0; kf_idx < track.keyframes.size(); ++kf_idx) {
        const auto& kf = track.keyframes[kf_idx];
        float x = pos.x + kf.time * size.x;
        float y = track_y + track_height * 0.5f;

        bool is_selected = (selected_track_index == static_cast<int>(track_idx) &&
          selected_keyframe_index == static_cast<int>(kf_idx));

        // Diamond shape
        float diamond_size = is_selected ? 7.0f : 5.0f;
        ImVec2 points[4] = {
          ImVec2(x, y - diamond_size),
          ImVec2(x + diamond_size, y),
          ImVec2(x, y + diamond_size),
          ImVec2(x - diamond_size, y)
        };

        ImU32 kf_color = is_selected ? IM_COL32(255, 255, 255, 255) : track_color;
        draw_list->AddConvexPolyFilled(points, 4, kf_color);

        // Make clickable
        ImGui::SetCursorScreenPos(ImVec2(x - diamond_size, y - diamond_size));
        ImGui::PushID(static_cast<int>(track_idx * 1000 + kf_idx));
        if (ImGui::InvisibleButton("##kf", ImVec2(diamond_size * 2, diamond_size * 2))) {
          selected_track_index = static_cast<int>(track_idx);
          selected_keyframe_index = static_cast<int>(kf_idx);
        }

        // Drag to reposition
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
          float new_x = ImGui::GetMousePos().x;
          float new_time = (new_x - pos.x) / size.x;
          anim->tracks[track_idx].keyframes[kf_idx].time = std::clamp(new_time, 0.0f, 1.0f);
        }
        ImGui::PopID();
      }
    }

    // Draw playhead
    float playhead_x = pos.x + preview_normalized_time * size.x;
    draw_list->AddLine(ImVec2(playhead_x, pos.y), ImVec2(playhead_x, pos.y + size.y),
      IM_COL32(255, 100, 100, 255), 2.0f);

    // Playhead triangle
    draw_list->AddTriangleFilled(
      ImVec2(playhead_x - 6, pos.y),
      ImVec2(playhead_x + 6, pos.y),
      ImVec2(playhead_x, pos.y + 10),
      IM_COL32(255, 100, 100, 255)
    );
  }
}

namespace AnimatorEditor {
  void Open(Engine::ECS::Entity entity) {
    selected_entity_id = static_cast<s32>(entity.get_id());
    current_mode = EditorMode::Entity;
    selected_track_index = -1;
    selected_keyframe_index = -1;
  }

  void Open(Engine::ECS::Prefab prefab, const std::string& prefab_path) {
    selected_prefab = prefab;
    selected_prefab_path = prefab_path;
    current_mode = EditorMode::Prefab;
    selected_track_index = -1;
    selected_keyframe_index = -1;
  }
}

void AnimatorEditorPanel()
{
  init_selection_listener();

  ImGui::Begin("Animator Editor");

  if (current_mode == EditorMode::None) {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Select an entity with Animator component");
    ImGui::End();
    return;
  }

  // Get the animator component
  Components::Animator* anim = nullptr;
  Engine::ECS::Entity entity;

  if (current_mode == EditorMode::Entity) {
    entity = Engine::ECS::get_entity(static_cast<Engine::ECS::EntityID>(selected_entity_id));
    if (!entity.is_valid()) {
      ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Invalid entity selected");
      ImGui::End();
      return;
    }

    anim = entity.get_component<Components::Animator>();
  }
  else if (current_mode == EditorMode::Prefab) {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Prefab editing not yet implemented");
    ImGui::End();
    return;
  }

  if (!anim) {
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Entity has no Animator component");

    if (ImGui::Button("Add Animator Component")) {
      entity.add_component<Components::Animator>();
    }
    ImGui::End();
    return;
  }

  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  // Calculate current normalized time
  float current_normalized_time = 0.0f;
  if (anim->duration > 0.0f) {
    current_normalized_time = anim->elapsed / anim->duration;
    if (anim->is_looping) {
      current_normalized_time = fmod(current_normalized_time, 1.0f);
    }
    else {
      current_normalized_time = std::clamp(current_normalized_time, 0.0f, 1.0f);
    }
    if (anim->is_reversed) {
      current_normalized_time = 1.0f - current_normalized_time;
    }
  }

  // ============================================================================
  // Animation State
  // ============================================================================
  ImGui::Text("Animation State");
  ImGui::Separator();

  ImGui::Text("Elapsed: %.2fs / %.2fs (%.1f%%)",
    anim->elapsed, anim->duration, current_normalized_time * 100.0f);

  ImGui::SameLine();
  if (anim->is_playing) {
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[Playing]");
  }
  else {
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "[Stopped]");
  }

  if (ImGui::Button("Reset")) {
    anim->elapsed = 0.0f;
    anim->start_time = Engine::Time::get_time();
    anim->is_reversed = false;
    anim->has_initialized = false;
  }
  ImGui::SameLine();
  if (ImGui::Button(anim->is_playing ? "Pause" : "Play")) {
    anim->is_playing = !anim->is_playing;
    if (anim->is_playing) {
      anim->start_time = Engine::Time::get_time() - anim->elapsed;
    }
  }

  ImGui::Spacing();

  // ============================================================================
  // Animation Settings
  // ============================================================================
  ImGui::Text("Settings");
  ImGui::Separator();

  ImGui::DragFloat("Duration (s)", &anim->duration, 0.1f, 0.1f, 60.0f, "%.2f");
  ImGui::Checkbox("Looping", &anim->is_looping);
  ImGui::SameLine();
  ImGui::Checkbox("Ping Pong", &anim->ping_pong);

  ImGui::Spacing();

  // ============================================================================
  // Timeline View
  // ============================================================================
  ImGui::Text("Timeline (click/drag to scrub)");
  ImGui::Separator();

  float timeline_height = 30.0f + anim->tracks.size() * 20.0f;
  timeline_height = std::max(60.0f, std::min(timeline_height, 200.0f));

  ImVec2 timeline_pos = ImGui::GetCursorScreenPos();
  ImVec2 timeline_size = ImVec2(ImGui::GetContentRegionAvail().x, timeline_height);

  ImGui::Dummy(timeline_size);

  DrawTimeline(draw_list, timeline_pos, timeline_size, anim, current_normalized_time);

  // Scrubbing
  ImGui::SetCursorScreenPos(timeline_pos);
  ImGui::InvisibleButton("##timeline_scrub", timeline_size);
  if (ImGui::IsItemActive()) {
    float click_x = ImGui::GetMousePos().x - timeline_pos.x;
    float new_time = std::clamp(click_x / timeline_size.x, 0.0f, 1.0f);

    anim->elapsed = new_time * anim->duration;
    anim->start_time = Engine::Time::get_time() - anim->elapsed;
    anim->is_reversed = false;

    Engine::ECS::request_entity_update(entity.get_id());
  }

  ImGui::Spacing();

  // ============================================================================
  // Tracks List
  // ============================================================================
  ImGui::Text("Tracks (%zu)", anim->tracks.size());
  ImGui::Separator();

  if (ImGui::Button("Add Track")) {
    Components::AnimationTrack new_track;
    new_track.component_type = "Components::Color";
    new_track.field_path = "r";
    new_track.keyframes.push_back({ 0.0f, 0.0f, "EaseLinearNone" });
    new_track.keyframes.push_back({ 1.0f, 1.0f, "EaseLinearNone" });
    anim->tracks.push_back(new_track);
    selected_track_index = static_cast<int>(anim->tracks.size()) - 1;
    selected_keyframe_index = -1;
  }
  ImGui::SameLine();
  if (selected_track_index >= 0 && selected_track_index < static_cast<int>(anim->tracks.size())) {
    if (ImGui::Button("Delete Track")) {
      anim->tracks.erase(anim->tracks.begin() + selected_track_index);
      selected_track_index = -1;
      selected_keyframe_index = -1;
    }
  }

  // Track list
  ImGui::BeginChild("TrackList", ImVec2(0, 100), true);
  for (size_t i = 0; i < anim->tracks.size(); ++i) {
    auto& track = anim->tracks[i];
    bool is_selected = (selected_track_index == static_cast<int>(i));

    ImGui::PushID(static_cast<int>(i));

    // Colored indicator
    ImU32 track_color = GetTrackColor(static_cast<int>(i));
    ImVec2 indicator_pos = ImGui::GetCursorScreenPos();
    draw_list->AddRectFilled(indicator_pos, ImVec2(indicator_pos.x + 4, indicator_pos.y + 16), track_color);
    ImGui::Dummy(ImVec2(8, 0));
    ImGui::SameLine();

    if (ImGui::Selectable(("##track" + std::to_string(i)).c_str(), is_selected, 0, ImVec2(0, 18))) {
      selected_track_index = static_cast<int>(i);
      selected_keyframe_index = -1;
    }
    ImGui::SameLine();
    ImGui::Text("%s -> %s (%zu kf)",
      track.component_type.c_str(), track.field_path.c_str(), track.keyframes.size());

    ImGui::PopID();
  }
  ImGui::EndChild();

  ImGui::Spacing();

  // ============================================================================
  // Selected Track Editor
  // ============================================================================
  if (selected_track_index >= 0 && selected_track_index < static_cast<int>(anim->tracks.size())) {
    auto& track = anim->tracks[selected_track_index];

    ImGui::Text("Edit Track %d", selected_track_index);
    ImGui::Separator();

    // Component type dropdown
    if (ImGui::BeginCombo("Component", track.component_type.c_str())) {
      for (const auto& comp : common_component_types) {
        if (ImGui::Selectable(comp.c_str(), track.component_type == comp)) {
          track.component_type = comp;
          // Reset field path to first option
          auto it = common_field_paths.find(comp);
          if (it != common_field_paths.end() && !it->second.empty()) {
            track.field_path = it->second[0];
          }
        }
      }
      ImGui::EndCombo();
    }

    // Field path dropdown
    auto it = common_field_paths.find(track.component_type);
    if (it != common_field_paths.end()) {
      if (ImGui::BeginCombo("Field", track.field_path.c_str())) {
        for (const auto& field : it->second) {
          if (ImGui::Selectable(field.c_str(), track.field_path == field)) {
            track.field_path = field;
          }
        }
        ImGui::EndCombo();
      }
    }
    else {
      // Custom field input
      static char field_buffer[128] = "";
      strncpy_s(field_buffer, track.field_path.c_str(), sizeof(field_buffer) - 1);
      if (ImGui::InputText("Field Path", field_buffer, sizeof(field_buffer))) {
        track.field_path = field_buffer;
      }
    }

    ImGui::Spacing();

    // Keyframes for this track
    ImGui::Text("Keyframes (%zu)", track.keyframes.size());

    if (ImGui::Button("Add Keyframe")) {
      Components::FloatKeyframe new_kf;
      if (track.keyframes.empty()) {
        new_kf.time = 0.0f;
        new_kf.value = 0.0f;
      }
      else {
        new_kf.time = std::min(track.keyframes.back().time + 0.25f, 1.0f);
        new_kf.value = track.keyframes.back().value;
      }
      track.keyframes.push_back(new_kf);
      selected_keyframe_index = static_cast<int>(track.keyframes.size()) - 1;
    }
    ImGui::SameLine();
    if (selected_keyframe_index >= 0 && selected_keyframe_index < static_cast<int>(track.keyframes.size())) {
      if (ImGui::Button("Delete Keyframe")) {
        track.keyframes.erase(track.keyframes.begin() + selected_keyframe_index);
        selected_keyframe_index = -1;
      }
    }

    // Keyframe list for this track
    ImGui::BeginChild("KeyframeList", ImVec2(0, 80), true);
    for (size_t i = 0; i < track.keyframes.size(); ++i) {
      auto& kf = track.keyframes[i];
      bool is_selected = (selected_keyframe_index == static_cast<int>(i));

      ImGui::PushID(static_cast<int>(i));
      if (ImGui::Selectable(("##kf" + std::to_string(i)).c_str(), is_selected, 0, ImVec2(0, 16))) {
        selected_keyframe_index = static_cast<int>(i);
      }
      ImGui::SameLine();
      ImGui::Text("t=%.2f | v=%.3f | %s", kf.time, kf.value, kf.easing.c_str());
      ImGui::PopID();
    }
    ImGui::EndChild();

    // Selected keyframe editor
    if (selected_keyframe_index >= 0 && selected_keyframe_index < static_cast<int>(track.keyframes.size())) {
      auto& kf = track.keyframes[selected_keyframe_index];

      ImGui::Spacing();
      ImGui::Text("Edit Keyframe %d", selected_keyframe_index);
      ImGui::Separator();

      ImGui::DragFloat("Time", &kf.time, 0.01f, 0.0f, 1.0f, "%.3f");
      ImGui::DragFloat("Value", &kf.value, 0.01f, -10000.0f, 10000.0f, "%.4f");

      ImGui::SetNextItemWidth(200);
      EasingCombo("Easing", kf.easing);

      // Curve preview
      ImVec2 curve_pos = ImGui::GetCursorScreenPos();
      ImGui::Dummy(ImVec2(80, 40));
      DrawEasingCurvePreview(draw_list, curve_pos, ImVec2(80, 40), kf.easing);

      // Sort keyframes after editing time
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        float selected_time = kf.time;
        std::sort(track.keyframes.begin(), track.keyframes.end(),
          [](const auto& a, const auto& b) { return a.time < b.time; });

        // Find the keyframe again
        for (size_t i = 0; i < track.keyframes.size(); ++i) {
          if (track.keyframes[i].time == selected_time) {
            selected_keyframe_index = static_cast<int>(i);
            break;
          }
        }
      }
    }
  }

  ImGui::End();
}
