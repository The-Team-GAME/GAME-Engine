//-----------------------------------------------------------------------------------
// file:    TransformAnimationEditorPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the TransformAnimationEditorPanel ImGui interface for
//          visually editing TransformAnimation keyframes with a timeline, curve
//          preview, and easing function selection.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "TransformAnimationEditorPanel.h"
#include "../Precompiled.h"
#include "../ECS/ECS.h"
#include "../Components/SpatialComponents.h"
#include "../Components/GraphicsComponents.h"
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
  int selected_keyframe_index = -1;

  // Preview state
  bool preview_playing = false;
  float preview_time = 0.0f;
  float preview_start_real_time = 0.0f;

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

  void init_selection_listener() {
    if (selection_initialized) return;
    selection_initialized = true;

    Engine::EventManager::add_event_listener<Engine::Events::SelectEntityEvent>(
      [](const Engine::Events::SelectEntityEvent& event) {
        if (event.entity_id > 0) {
          selected_entity_id = static_cast<s32>(event.entity_id);
          current_mode = EditorMode::Entity;
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
    prev_point.y = pos.y + size.y; // Start at bottom-left (0,0)

    for (int i = 1; i <= segments; ++i) {
      float t = static_cast<float>(i) / segments;
      float value = easing_func(t, 0.0f, 1.0f, 1.0f);

      ImVec2 point;
      point.x = pos.x + t * size.x;
      point.y = pos.y + size.y - value * size.y; // Flip Y (0 at bottom)

      draw_list->AddLine(prev_point, point, IM_COL32(100, 200, 255, 255), 2.0f);
      prev_point = point;
    }
  }

  // Draw timeline with keyframes
  void DrawTimeline(ImDrawList* draw_list, ImVec2 pos, ImVec2 size,
    Components::TransformAnimation* anim, float preview_normalized_time) {
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

    // Draw keyframes
    float keyframe_y = pos.y + size.y * 0.6f;
    for (size_t i = 0; i < anim->keyframes.size(); ++i) {
      const auto& kf = anim->keyframes[i];
      float x = pos.x + kf.time * size.x;

      bool is_selected = (selected_keyframe_index == static_cast<int>(i));
      ImU32 color = is_selected ? IM_COL32(255, 200, 50, 255) : IM_COL32(100, 200, 255, 255);

      // Diamond shape for keyframe
      float diamond_size = is_selected ? 8.0f : 6.0f;
      ImVec2 points[4] = {
        ImVec2(x, keyframe_y - diamond_size),
        ImVec2(x + diamond_size, keyframe_y),
        ImVec2(x, keyframe_y + diamond_size),
        ImVec2(x - diamond_size, keyframe_y)
      };
      draw_list->AddConvexPolyFilled(points, 4, color);
      draw_list->AddPolyline(points, 4, IM_COL32(255, 255, 255, 200), ImDrawFlags_Closed, 1.0f);

      // Make keyframe clickable
      ImGui::SetCursorScreenPos(ImVec2(x - diamond_size, keyframe_y - diamond_size));
      ImGui::PushID(static_cast<int>(i));
      if (ImGui::InvisibleButton("##keyframe", ImVec2(diamond_size * 2, diamond_size * 2))) {
        selected_keyframe_index = static_cast<int>(i);
      }

      // Drag to reposition time
      if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        float new_x = ImGui::GetMousePos().x;
        float new_time = (new_x - pos.x) / size.x;
        anim->keyframes[i].time = std::clamp(new_time, 0.0f, 1.0f);
      }
      ImGui::PopID();
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

namespace TransformAnimationEditor {
  void Open(Engine::ECS::Entity entity) {
    selected_entity_id = static_cast<s32>(entity.get_id());
    current_mode = EditorMode::Entity;
    selected_keyframe_index = -1;
  }

  void Open(Engine::ECS::Prefab prefab, const std::string& prefab_path) {
    selected_prefab = prefab;
    selected_prefab_path = prefab_path;
    current_mode = EditorMode::Prefab;
    selected_keyframe_index = -1;
  }
}

void TransformAnimationEditorPanel()
{
  init_selection_listener();

  ImGui::Begin("Transform Animation Editor");

  if (current_mode == EditorMode::None) {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Select an entity with TransformAnimation component");
    ImGui::End();
    return;
  }

  // Get the animation component
  Components::TransformAnimation* anim = nullptr;
  Components::Transform2D* transform = nullptr;
  Engine::ECS::Entity entity;

  if (current_mode == EditorMode::Entity) {
    entity = Engine::ECS::get_entity(static_cast<Engine::ECS::EntityID>(selected_entity_id));
    if (!entity.is_valid()) {
      ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Invalid entity selected");
      ImGui::End();
      return;
    }

    anim = entity.get_component<Components::TransformAnimation>();
    transform = entity.get_component<Components::Transform2D>();
  }
  else if (current_mode == EditorMode::Prefab) {
    // TODO: Implement prefab mode
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Prefab editing not yet implemented");
    ImGui::End();
    return;
  }

  if (!anim) {
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Entity has no TransformAnimation component");

    if (ImGui::Button("Add TransformAnimation Component")) {
      entity.add_component<Components::TransformAnimation>();
    }
    ImGui::End();
    return;
  }

  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  // ============================================================================
  // Playback Controls
  // ============================================================================
  ImGui::Text("Playback Controls");
  ImGui::Separator();

  if (ImGui::Button(preview_playing ? "Pause" : "Play")) {
    preview_playing = !preview_playing;
    if (preview_playing) {
      preview_start_real_time = Engine::Time::get_unscaled_time() - preview_time * anim->duration;
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Stop")) {
    preview_playing = false;
    preview_time = 0.0f;
    // Reset animation
    anim->elapsed = 0.0f;
    anim->start_time = Engine::Time::get_time();
    anim->is_reversed = false;
    anim->has_initialized = false;
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    anim->elapsed = 0.0f;
    anim->start_time = Engine::Time::get_time();
    anim->is_reversed = false;
    anim->has_initialized = false;
  }

  // Update preview time
  if (preview_playing) {
    float real_elapsed = Engine::Time::get_unscaled_time() - preview_start_real_time;
    preview_time = fmod(real_elapsed / anim->duration, 1.0f);
  }

  // Timeline scrubber
  ImGui::SliderFloat("Preview Time", &preview_time, 0.0f, 1.0f, "%.2f");

  ImGui::Spacing();

  // ============================================================================
  // Animation Settings
  // ============================================================================
  ImGui::Text("Animation Settings");
  ImGui::Separator();

  ImGui::DragFloat("Duration (s)", &anim->duration, 0.1f, 0.1f, 60.0f, "%.2f");
  ImGui::Checkbox("Looping", &anim->is_looping);
  ImGui::SameLine();
  ImGui::Checkbox("Ping Pong", &anim->ping_pong);
  ImGui::SameLine();
  ImGui::Checkbox("Relative Mode", &anim->use_relative);

  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("When enabled, keyframe values are added to the entity's initial transform");
  }

  ImGui::Checkbox("Is Playing", &anim->is_playing);

  ImGui::Spacing();

  // ============================================================================
  // Timeline View
  // ============================================================================
  ImGui::Text("Timeline");
  ImGui::Separator();

  ImVec2 timeline_pos = ImGui::GetCursorScreenPos();
  ImVec2 timeline_size = ImVec2(ImGui::GetContentRegionAvail().x, 60.0f);

  // Reserve space for timeline
  ImGui::Dummy(timeline_size);

  DrawTimeline(draw_list, timeline_pos, timeline_size, anim, preview_time);

  // Click on timeline to set preview time
  ImGui::SetCursorScreenPos(timeline_pos);
  ImGui::InvisibleButton("##timeline_click", timeline_size);
  if (ImGui::IsItemClicked()) {
    float click_x = ImGui::GetMousePos().x - timeline_pos.x;
    preview_time = std::clamp(click_x / timeline_size.x, 0.0f, 1.0f);
  }

  ImGui::Spacing();

  // ============================================================================
  // Keyframe List
  // ============================================================================
  ImGui::Text("Keyframes (%zu)", anim->keyframes.size());
  ImGui::Separator();

  if (ImGui::Button("Add Keyframe")) {
    Components::TransformKeyframe new_kf;
    if (transform) {
      new_kf.position = transform->position;
      new_kf.rotation = transform->rotation;
      new_kf.scale = transform->scale;
    }
    // Find a good time position
    if (anim->keyframes.empty()) {
      new_kf.time = 0.0f;
    }
    else {
      new_kf.time = std::min(anim->keyframes.back().time + 0.25f, 1.0f);
    }
    anim->keyframes.push_back(new_kf);
    selected_keyframe_index = static_cast<int>(anim->keyframes.size()) - 1;
  }
  ImGui::SameLine();
  if (ImGui::Button("Add from Current Transform") && transform) {
    Components::TransformKeyframe new_kf;
    new_kf.position = transform->position;
    new_kf.rotation = transform->rotation;
    new_kf.scale = transform->scale;
    new_kf.time = preview_time;
    anim->keyframes.push_back(new_kf);
    selected_keyframe_index = static_cast<int>(anim->keyframes.size()) - 1;

    // Sort keyframes by time
    std::sort(anim->keyframes.begin(), anim->keyframes.end(),
      [](const auto& a, const auto& b) { return a.time < b.time; });
  }
  ImGui::SameLine();
  if (selected_keyframe_index >= 0 && selected_keyframe_index < static_cast<int>(anim->keyframes.size())) {
    if (ImGui::Button("Delete Selected")) {
      anim->keyframes.erase(anim->keyframes.begin() + selected_keyframe_index);
      selected_keyframe_index = -1;
    }
  }

  // Keyframe list
  ImGui::BeginChild("KeyframeList", ImVec2(0, 150), true);
  for (size_t i = 0; i < anim->keyframes.size(); ++i) {
    auto& kf = anim->keyframes[i];
    bool is_selected = (selected_keyframe_index == static_cast<int>(i));

    ImGui::PushID(static_cast<int>(i));
    if (ImGui::Selectable(("##kf" + std::to_string(i)).c_str(), is_selected, 0, ImVec2(0, 20))) {
      selected_keyframe_index = static_cast<int>(i);
    }
    ImGui::SameLine();
    ImGui::Text("KF %zu | t=%.2f | pos(%.0f, %.0f) | rot=%.1f | scale(%.2f, %.2f)",
      i, kf.time, kf.position.x, kf.position.y, kf.rotation, kf.scale.x, kf.scale.y);
    ImGui::PopID();
  }
  ImGui::EndChild();

  ImGui::Spacing();

  // ============================================================================
  // Selected Keyframe Editor
  // ============================================================================
  if (selected_keyframe_index >= 0 && selected_keyframe_index < static_cast<int>(anim->keyframes.size())) {
    auto& kf = anim->keyframes[selected_keyframe_index];

    ImGui::Text("Edit Keyframe %d", selected_keyframe_index);
    ImGui::Separator();

    // Time
    ImGui::DragFloat("Time", &kf.time, 0.01f, 0.0f, 1.0f, "%.3f");

    ImGui::Spacing();

    // Position
    ImGui::Text("Position");
    float pos[2] = { kf.position.x, kf.position.y };
    if (ImGui::DragFloat2("##Position", pos, 1.0f)) {
      kf.position.x = pos[0];
      kf.position.y = pos[1];
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    EasingCombo("##PosEasing", kf.position_easing);

    // Mini curve preview for position
    ImVec2 curve_preview_pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(60, 30));
    DrawEasingCurvePreview(draw_list, curve_preview_pos, ImVec2(60, 30), kf.position_easing);

    ImGui::Spacing();

    // Rotation
    ImGui::Text("Rotation");
    ImGui::DragFloat("##Rotation", &kf.rotation, 1.0f, -360.0f, 360.0f, "%.1f deg");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    EasingCombo("##RotEasing", kf.rotation_easing);

    // Mini curve preview for rotation
    curve_preview_pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(60, 30));
    DrawEasingCurvePreview(draw_list, curve_preview_pos, ImVec2(60, 30), kf.rotation_easing);

    ImGui::Spacing();

    // Scale
    ImGui::Text("Scale");
    float scale[2] = { kf.scale.x, kf.scale.y };
    if (ImGui::DragFloat2("##Scale", scale, 0.01f, 0.01f, 10.0f, "%.3f")) {
      kf.scale.x = scale[0];
      kf.scale.y = scale[1];
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    EasingCombo("##ScaleEasing", kf.scale_easing);

    // Mini curve preview for scale
    curve_preview_pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(60, 30));
    DrawEasingCurvePreview(draw_list, curve_preview_pos, ImVec2(60, 30), kf.scale_easing);

    ImGui::Spacing();

    // Copy current transform to keyframe
    if (transform && ImGui::Button("Copy Current Transform")) {
      kf.position = transform->position;
      kf.rotation = transform->rotation;
      kf.scale = transform->scale;
    }

    // Sort keyframes after editing time
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      // Remember which keyframe we had selected by its content
      Components::TransformKeyframe selected_kf = kf;

      std::sort(anim->keyframes.begin(), anim->keyframes.end(),
        [](const auto& a, const auto& b) { return a.time < b.time; });

      // Find the keyframe again
      for (size_t i = 0; i < anim->keyframes.size(); ++i) {
        if (anim->keyframes[i].time == selected_kf.time) {
          selected_keyframe_index = static_cast<int>(i);
          break;
        }
      }
    }
  }

  ImGui::End();
}
