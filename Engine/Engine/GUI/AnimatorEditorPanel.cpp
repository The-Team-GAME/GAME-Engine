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

  // Popup state
  bool show_add_track_popup = false;
  bool show_edit_track_popup = false;
  int editing_track_index = -1;
  std::string popup_component_type = "Components::Color";
  std::string popup_field_path = "r";

  // Playback state
  bool is_playing = false;

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

  // Helper to read a field value from an entity's component using reflection
  float get_field_value(Engine::ECS::Entity entity, const std::string& component_type, const std::string& field_path) {
    auto& containers = Engine::ECS::Internal::get_containers();

    // Find the component array
    for (auto& [type_id, array] : containers.component_arrays) {
      if (array->get_type_name() != component_type) continue;
      if (!array->is_reflectable()) return 0.0f;

      auto generic = array->to_generic_value(entity.get_id());
      if (!generic.isObject()) return 0.0f;

      // Parse field path (e.g., "position.x")
      std::vector<std::string> parts;
      size_t start = 0, pos = 0;
      while ((pos = field_path.find('.', start)) != std::string::npos) {
        parts.push_back(field_path.substr(start, pos - start));
        start = pos + 1;
      }
      parts.push_back(field_path.substr(start));

      // Navigate to the field
      if (parts.size() == 1) {
        if (generic.has(parts[0])) {
          ReflectionLibrary::GenericValue val = generic[parts[0]].get();
          if (val.isDouble()) return static_cast<float>(val.getDouble());
          if (val.isInt()) return static_cast<float>(val.getInt());
        }
      }
      else if (parts.size() == 2) {
        if (generic.has(parts[0])) {
          ReflectionLibrary::GenericValue nested = generic[parts[0]].get();
          if (nested.isObject() && nested.has(parts[1])) {
            ReflectionLibrary::GenericValue val = nested[parts[1]].get();
            if (val.isDouble()) return static_cast<float>(val.getDouble());
            if (val.isInt()) return static_cast<float>(val.getInt());
          }
        }
      }
      break;
    }
    return 0.0f;
  }

  // Helper to update entity without moving the playhead
  // Uses force_update flag to apply animation at current elapsed without recalculating
  void update_entity_preserve_playhead(Engine::ECS::Entity entity, Components::Animator* anim) {
    anim->force_update = true;
    Engine::ECS::request_entity_update(entity.get_id());
  }

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
  // label_width: width reserved for track labels on the left
  void DrawTimeline(ImDrawList* draw_list, ImVec2 pos, ImVec2 size, float label_width,
    Components::Animator* anim, float preview_normalized_time, Engine::ECS::Entity entity) {
    
    float timeline_x = pos.x + label_width;
    float timeline_width = size.x - label_width;
    
    // Background for timeline area
    draw_list->AddRectFilled(ImVec2(timeline_x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(30, 30, 35, 255));
    draw_list->AddRect(ImVec2(timeline_x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(80, 80, 80, 255));
    
    // Background for label area
    draw_list->AddRectFilled(pos, ImVec2(timeline_x, pos.y + size.y), IM_COL32(25, 25, 30, 255));
    draw_list->AddRect(pos, ImVec2(timeline_x, pos.y + size.y), IM_COL32(60, 60, 60, 255));

    // Draw time markers
    for (int i = 0; i <= 10; ++i) {
      float t = i / 10.0f;
      float x = timeline_x + t * timeline_width;
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
        ImVec2(timeline_x, track_y),
        ImVec2(pos.x + size.x, track_y + track_height - 2),
        bg_color
      );
      
      // Label background
      ImU32 label_bg = is_selected_track ? IM_COL32(45, 45, 55, 255) : IM_COL32(30, 30, 35, 255);
      draw_list->AddRectFilled(
        ImVec2(pos.x, track_y),
        ImVec2(timeline_x, track_y + track_height - 2),
        label_bg
      );

      // Track label - clickable for selection, right-click for edit
      std::string label = track.field_path;
      if (label.length() > 10) {
        label = label.substr(0, 8) + "..";
      }
      draw_list->AddText(ImVec2(pos.x + 4, track_y + 2), track_color, label.c_str());
      
      // Make label clickable
      ImGui::SetCursorScreenPos(ImVec2(pos.x, track_y));
      ImGui::PushID(static_cast<int>(track_idx + 10000));
      if (ImGui::InvisibleButton("##track_label", ImVec2(label_width, track_height - 2))) {
        selected_track_index = static_cast<int>(track_idx);
        selected_keyframe_index = -1;
      }
      // Right-click to edit track
      if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        editing_track_index = static_cast<int>(track_idx);
        popup_component_type = track.component_type;
        popup_field_path = track.field_path;
        show_edit_track_popup = true;
      }
      ImGui::PopID();

      // Draw keyframes for this track
      for (size_t kf_idx = 0; kf_idx < track.keyframes.size(); ++kf_idx) {
        const auto& kf = track.keyframes[kf_idx];
        float x = timeline_x + kf.time * timeline_width;
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
          float new_time = (new_x - timeline_x) / timeline_width;
          anim->tracks[track_idx].keyframes[kf_idx].time = std::clamp(new_time, 0.0f, 1.0f);
          
          // Update entity to reflect keyframe change (preserves playhead)
          update_entity_preserve_playhead(entity, anim);
        }
        ImGui::PopID();
      }
    }

    // Draw playhead
    float playhead_x = timeline_x + preview_normalized_time * timeline_width;
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
  // Playback Controls
  // ============================================================================
  if (is_playing) {
    if (ImGui::Button("Stop", ImVec2(60, 0))) {
      is_playing = false;
    }
  } else {
    if (ImGui::Button("Play", ImVec2(60, 0))) {
      is_playing = true;
      anim->start_time = Engine::Time::get_time() - anim->elapsed;
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset", ImVec2(60, 0))) {
    anim->elapsed = 0.0f;
    anim->start_time = Engine::Time::get_time();
    anim->is_reversed = false;
    Engine::ECS::request_entity_update(entity.get_id());
  }
  
  // Update animation when playing
  if (is_playing) {
    anim->elapsed = Engine::Time::get_time() - anim->start_time;
    Engine::ECS::request_entity_update(entity.get_id());
  }

  ImGui::Spacing();

  // ============================================================================
  // Timeline Controls (always visible above timeline)
  // ============================================================================
  bool has_track_selected = selected_track_index >= 0 && selected_track_index < static_cast<int>(anim->tracks.size());
  bool has_keyframe_selected = has_track_selected && selected_keyframe_index >= 0 && 
    selected_keyframe_index < static_cast<int>(anim->tracks[selected_track_index].keyframes.size());

  // Add Track button - always enabled
  if (ImGui::Button("Add Track")) {
    popup_component_type = "Components::Color";
    popup_field_path = "r";
    show_add_track_popup = true;
  }
  
  ImGui::SameLine();
  
  // Delete Track button - disabled if no track selected
  ImGui::BeginDisabled(!has_track_selected);
  if (ImGui::Button("Delete Track")) {
    anim->tracks.erase(anim->tracks.begin() + selected_track_index);
    selected_track_index = -1;
    selected_keyframe_index = -1;
  }
  ImGui::EndDisabled();
  
  ImGui::SameLine();
  
  // Add Keyframe button - disabled if no track selected
  ImGui::BeginDisabled(!has_track_selected);
  if (ImGui::Button("Add Keyframe") && has_track_selected) {
    auto& track = anim->tracks[selected_track_index];
    Components::FloatKeyframe new_kf;
    float current_value = get_field_value(entity, track.component_type, track.field_path);
    new_kf.time = current_normalized_time;
    new_kf.value = current_value;
    track.keyframes.push_back(new_kf);
    std::sort(track.keyframes.begin(), track.keyframes.end(),
      [](const auto& a, const auto& b) { return a.time < b.time; });
    for (size_t i = 0; i < track.keyframes.size(); ++i) {
      if (track.keyframes[i].time == new_kf.time && track.keyframes[i].value == new_kf.value) {
        selected_keyframe_index = static_cast<int>(i);
        break;
      }
    }
  }
  ImGui::EndDisabled();
  
  ImGui::SameLine();
  
  // Delete Keyframe button - disabled if no keyframe selected
  ImGui::BeginDisabled(!has_keyframe_selected);
  if (ImGui::Button("Delete Keyframe") && has_keyframe_selected) {
    auto& track = anim->tracks[selected_track_index];
    track.keyframes.erase(track.keyframes.begin() + selected_keyframe_index);
    selected_keyframe_index = -1;
  }
  ImGui::EndDisabled();

  // ============================================================================
  // Timeline View
  // ============================================================================
  float label_width = 80.0f;
  float timeline_height = 30.0f + anim->tracks.size() * 20.0f;
  timeline_height = std::max(60.0f, std::min(timeline_height, 200.0f));

  ImVec2 timeline_pos = ImGui::GetCursorScreenPos();
  ImVec2 timeline_size = ImVec2(ImGui::GetContentRegionAvail().x, timeline_height);

  ImGui::Dummy(timeline_size);

  DrawTimeline(draw_list, timeline_pos, timeline_size, label_width, anim, current_normalized_time, entity);

  // Scrubbing - only cover the timeline area, not the label area
  float timeline_x_start = timeline_pos.x + label_width;
  float timeline_actual_width = timeline_size.x - label_width;
  ImGui::SetCursorScreenPos(ImVec2(timeline_x_start, timeline_pos.y));
  ImGui::InvisibleButton("##timeline_scrub", ImVec2(timeline_actual_width, timeline_size.y));
  if (ImGui::IsItemActive()) {
    float click_x = ImGui::GetMousePos().x - timeline_x_start;
    float new_time = std::clamp(click_x / timeline_actual_width, 0.0f, 1.0f);

    anim->elapsed = new_time * anim->duration;
    anim->start_time = Engine::Time::get_time() - anim->elapsed;
    anim->is_reversed = false;

    Engine::ECS::request_entity_update(entity.get_id());
  }

  ImGui::Spacing();

  // ============================================================================
  // Add Track Popup
  // ============================================================================
  if (show_add_track_popup) {
    ImGui::OpenPopup("Add Track##popup");
    show_add_track_popup = false;
  }
  if (ImGui::BeginPopup("Add Track##popup")) {
    ImGui::Text("Add New Track");
    ImGui::Separator();
    
    // Component type dropdown
    if (ImGui::BeginCombo("Component", popup_component_type.c_str())) {
      for (const auto& comp : common_component_types) {
        if (ImGui::Selectable(comp.c_str(), popup_component_type == comp)) {
          popup_component_type = comp;
          // Reset field path to first option
          auto it = common_field_paths.find(comp);
          if (it != common_field_paths.end() && !it->second.empty()) {
            popup_field_path = it->second[0];
          }
        }
      }
      ImGui::EndCombo();
    }

    // Field path dropdown
    auto it = common_field_paths.find(popup_component_type);
    if (it != common_field_paths.end()) {
      if (ImGui::BeginCombo("Field", popup_field_path.c_str())) {
        for (const auto& field : it->second) {
          if (ImGui::Selectable(field.c_str(), popup_field_path == field)) {
            popup_field_path = field;
          }
        }
        ImGui::EndCombo();
      }
    }

    ImGui::Spacing();
    if (ImGui::Button("Create Track")) {
      Components::AnimationTrack new_track;
      new_track.component_type = popup_component_type;
      new_track.field_path = popup_field_path;
      
      float current_value = get_field_value(entity, new_track.component_type, new_track.field_path);
      
      new_track.keyframes.push_back({ current_value, 0.0f, "EaseLinearNone" });
      new_track.keyframes.push_back({ current_value, 1.0f, "EaseLinearNone" });
      anim->tracks.push_back(new_track);
      selected_track_index = static_cast<int>(anim->tracks.size()) - 1;
      selected_keyframe_index = -1;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  // ============================================================================
  // Edit Track Popup (right-click context menu)
  // ============================================================================
  if (show_edit_track_popup) {
    ImGui::OpenPopup("Edit Track##popup");
    show_edit_track_popup = false;
  }
  if (ImGui::BeginPopup("Edit Track##popup")) {
    ImGui::Text("Edit Track");
    ImGui::Separator();
    
    // Component type dropdown
    if (ImGui::BeginCombo("Component", popup_component_type.c_str())) {
      for (const auto& comp : common_component_types) {
        if (ImGui::Selectable(comp.c_str(), popup_component_type == comp)) {
          popup_component_type = comp;
          // Reset field path to first option
          auto it = common_field_paths.find(comp);
          if (it != common_field_paths.end() && !it->second.empty()) {
            popup_field_path = it->second[0];
          }
        }
      }
      ImGui::EndCombo();
    }

    // Field path dropdown
    auto it = common_field_paths.find(popup_component_type);
    if (it != common_field_paths.end()) {
      if (ImGui::BeginCombo("Field", popup_field_path.c_str())) {
        for (const auto& field : it->second) {
          if (ImGui::Selectable(field.c_str(), popup_field_path == field)) {
            popup_field_path = field;
          }
        }
        ImGui::EndCombo();
      }
    }

    ImGui::Spacing();
    if (ImGui::Button("Apply")) {
      if (editing_track_index >= 0 && editing_track_index < static_cast<int>(anim->tracks.size())) {
        anim->tracks[editing_track_index].component_type = popup_component_type;
        anim->tracks[editing_track_index].field_path = popup_field_path;
      }
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete Track")) {
      if (editing_track_index >= 0 && editing_track_index < static_cast<int>(anim->tracks.size())) {
        anim->tracks.erase(anim->tracks.begin() + editing_track_index);
        if (selected_track_index == editing_track_index) {
          selected_track_index = -1;
          selected_keyframe_index = -1;
        }
      }
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  // ============================================================================
  // Selected Keyframe Editor
  // ============================================================================
  if (selected_track_index >= 0 && selected_track_index < static_cast<int>(anim->tracks.size())) {
    auto& track = anim->tracks[selected_track_index];

    // Selected keyframe editor
    if (selected_keyframe_index >= 0 && selected_keyframe_index < static_cast<int>(track.keyframes.size())) {
      auto& kf = track.keyframes[selected_keyframe_index];

      ImGui::Spacing();
      ImGui::Text("Keyframe %d", selected_keyframe_index);
      ImGui::Separator();

      if (ImGui::DragFloat("Time", &kf.time, 0.01f, 0.0f, 1.0f, "%.3f")) {
        update_entity_preserve_playhead(entity, anim);
      }
      if (ImGui::DragFloat("Value", &kf.value, 0.01f, -10000.0f, 10000.0f, "%.4f")) {
        update_entity_preserve_playhead(entity, anim);
      }

      ImGui::SetNextItemWidth(200);
      if (EasingCombo("Easing", kf.easing)) {
        update_entity_preserve_playhead(entity, anim);
      }

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
