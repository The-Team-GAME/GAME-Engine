//-----------------------------------------------------------------------------------
// file:    ColliderEditorPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Antigravity
//
// brief:   This file implements the ColliderEditorPanel ImGui interface for visualizing
//          and editing entity colliders. It renders the entity's sprite and overlays
//          the collider AABB, allowing for visual resizing.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ColliderEditorPanel.h"
#include <imgui.h>
#include "../ECS/ECS.h"
#include "../Components/CollisionComponents.h"
#include "../Components/StateMachineComponents.h"
#include "../Components/GraphicsComponents.h"
#include "../Components/SpatialComponents.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include "../Graphics/Renderer.h" // For Texture
#include "../InputManager.h"

namespace {
  s32 selected_entity_id = -1;
  bool selection_initialized = false;
  std::string preview_state_name = "";
  float preview_animation_start_time = 0.0f;

  void init_selection_listener() {
    if (selection_initialized) return;
    selection_initialized = true;

    Engine::EventManager::add_event_listener<Engine::Events::SelectEntityEvent>(
      [](const Engine::Events::SelectEntityEvent& event) {
        selected_entity_id = static_cast<s32>(event.entity_id);
      }
    );
  }

  // Helper to draw a handle and return true if dragged
  bool draw_handle(ImDrawList* draw_list, ImVec2 pos, float size, ImU32 color, int id, ImVec2& out_delta) {
    ImVec2 min = { pos.x - size / 2, pos.y - size / 2 };
    ImVec2 max = { pos.x + size / 2, pos.y + size / 2 };

    draw_list->AddRectFilled(min, max, color);
    draw_list->AddRect(min, max, IM_COL32(0, 0, 0, 255));

    ImGui::SetCursorScreenPos(min);
    ImGui::PushID(id);
    ImGui::InvisibleButton("##handle", { size, size });
    ImGui::PopID();

    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();

    if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      ImVec2 delta = ImGui::GetIO().MouseDelta;
      out_delta = delta;
      return true;
    }
    return false;
  }
}

void ColliderEditorPanel()
{
  init_selection_listener();

  ImGui::Begin("Collider Editor");

  if (selected_entity_id == -1) {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No entity selected");
    ImGui::End();
    return;
  }

  auto entity = Engine::ECS::get_entity(selected_entity_id);
  if (!entity.is_valid()) {
    selected_entity_id = -1;
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Selected entity no longer exists");
    ImGui::End();
    return;
  }

  auto* collider = entity.get_component<Components::BoxCollider>();
  auto* texture = entity.get_component<Components::Texture>();
  auto* state_machine = entity.get_component<Components::StateMachine>();

  if (!collider) {
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "Selected entity has no BoxCollider");
    if (ImGui::Button("Add BoxCollider")) {
      entity.add_component<Components::BoxCollider>(50.0f, 50.0f, false);
    }
    ImGui::End();
    return;
  }

  // --- State Selection Logic ---
  std::string current_editing_state = "";
  static std::string selected_state_name = ""; // Persist selection
  bool editing_specific_state = false;

  if (state_machine) {
    ImGui::Text("State Machine Detected");

    // Dropdown to select state
    if (ImGui::BeginCombo("Edit State", selected_state_name.empty() ? "Current (Live)" : selected_state_name.c_str())) {

      if (ImGui::Selectable("Current (Live)", selected_state_name.empty())) {
        selected_state_name = "";
      }

      for (auto& [name, state] : state_machine->states) {
        bool is_selected = (selected_state_name == name);
        if (ImGui::Selectable(name.c_str(), is_selected)) {
          selected_state_name = name;
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    if (!selected_state_name.empty()) {
      if (state_machine->states.find(selected_state_name) != state_machine->states.end()) {
        editing_specific_state = true;
        current_editing_state = selected_state_name;
      }
      else {
        // State might have been deleted
        selected_state_name = "";
      }
    }
  }

  // --- Collider Type Selection ---
  ImGui::Separator();
  ImGui::Text("Collider Type");

  // We'll read the current type and allow changing it
  static int current_type_index = 0; // 0=AABB, 1=Polygon
  static bool type_just_changed = false;
  const char* type_names[] = { "AABB", "Polygon" };

  if (ImGui::Combo("##ColliderType", &current_type_index, type_names, IM_ARRAYSIZE(type_names))) {
    // Type changed via combo box
    type_just_changed = true;
  }

  // --- Data Retrieval ---
  Engine::Graphics::Texture texture_handle = nullptr;
  const Components::TextureAtlas* texture_atlas = nullptr;
  const Components::Animation* animation = nullptr;

  float col_width = 0.0f;
  float col_height = 0.0f;
  Math::Vec2D col_offset = { 0.0f, 0.0f };
  Components::ColliderType col_type = Components::ColliderType::AABB;
  std::vector<Math::Vec2D> col_vertices; // For polygon colliders
  bool should_apply_flip = false; // Only apply flip in live mode

  if (editing_specific_state) {
    // STATE EDITING MODE - completely isolated from live entity
    auto& state = state_machine->states[current_editing_state];

    // Get texture from state animation if available
    if (state.animation.has_value()) {
      if (state.animation->texture.texture_handle) {
        texture_handle = state.animation->texture.texture_handle;
        texture_atlas = &state.animation->texture_atlas;
        animation = &state.animation->animation;

        // Track preview animation start time (reset when state changes)
        if (preview_state_name != current_editing_state) {
          preview_state_name = current_editing_state;
          preview_animation_start_time = Engine::Time::get_time();
        }
      }

      // Get collider data from state if available
      // Note: Will set defaults after sprite_world_width/height are calculated
      if (state.animation->collider_data.has_value()) {
        col_type = state.animation->collider_data->type;
        col_width = state.animation->collider_data->width;
        col_height = state.animation->collider_data->height;
        col_offset = state.animation->collider_data->offset;
        col_vertices = state.animation->collider_data->vertices;
      }
    }
    else {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "State has no animation data.");
    }
  }
  else {
    // LIVE EDITING MODE - use entity's current data including flip
    if (texture && texture->texture_handle) {
      texture_handle = texture->texture_handle;
      texture_atlas = entity.get_component<Components::TextureAtlas>();
      animation = entity.get_component<Components::Animation>();
    }

    col_type = collider->type;
    col_width = collider->width;
    col_height = collider->height;
    col_offset = collider->offset;
    col_vertices = collider->polygon.vertices;
    should_apply_flip = true; // Apply flip only in live mode
  }

  if (!texture_handle) {
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "No Texture to visualize");
    // Show numeric controls at least
    if (editing_specific_state) {
      auto& state = state_machine->states[current_editing_state];
      if (state.animation.has_value() && state.animation->collider_data.has_value()) {
        ImGui::DragFloat("Width", &state.animation->collider_data->width);
        ImGui::DragFloat("Height", &state.animation->collider_data->height);
        ImGui::DragFloat2("Offset", &state.animation->collider_data->offset.x);

        if (ImGui::Button("Remove Override")) {
          state.animation->collider_data.reset();
        }
      }
    }
    else {
      ImGui::DragFloat("Width", &collider->width);
      ImGui::DragFloat("Height", &collider->height);
      ImGui::DragFloat2("Offset", &collider->offset.x);
    }
    ImGui::End();
    return;
  }

  // --- Visualization Logic ---

  // Calculate UVs
  ImVec2 uv0 = { 0, 0 };
  ImVec2 uv1 = { 1, 1 };

  // Determine Sprite World Dimensions (Virtual Pixels)
  float sprite_world_width = 0.0f;
  float sprite_world_height = 0.0f;

  // 1. Base Scale from Transform2D
  if (auto* xform = entity.get_component<Components::Transform2D>()) {
    sprite_world_width = xform->scale.x;
    sprite_world_height = xform->scale.y;
  }
  else if (texture_handle) {
    // Fallback if no transform (unlikely for rendered entity)
    sprite_world_width = static_cast<float>(texture_handle->width);
    sprite_world_height = static_cast<float>(texture_handle->height);
  }

  // 2. State Override (only when editing specific state)
  if (editing_specific_state) {
    auto& state = state_machine->states[current_editing_state];
    if (state.animation.has_value() && state.animation->scale.has_value()) {
      sprite_world_width = state.animation->scale->x;
      sprite_world_height = state.animation->scale->y;
    }

    // Now set defaults for states without collider override
    if (!state.animation.has_value() || !state.animation->collider_data.has_value()) {
      col_width = sprite_world_width;
      col_height = sprite_world_height;
      col_offset = { 0.0f, 0.0f };

      if (state.animation.has_value()) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "No collider override for this state.");
        ImGui::Text("Default: %.0fx%.0f, offset (0, 0)", col_width, col_height);
        if (ImGui::Button("Create Override with Defaults")) {
          Components::ColliderData data;
          data.type = col_type;
          data.width = col_width;
          data.height = col_height;
          data.offset = col_offset;
          data.vertices = col_vertices;
          state.animation->collider_data = data;
        }
      }
    }
  }

  // 3. UV Calculation (for texture subset)
  if (texture_atlas && !texture_atlas->subTextures.empty()) {
    int atlas_index = 0;
    if (animation && !animation->frames.empty()) {
      if (editing_specific_state) {
        // Calculate animated frame using preview start time
        // Always loop in preview mode for better visualization
        float elapsed = Engine::Time::get_time() - preview_animation_start_time;
        float time_to_use = elapsed;

        if (animation->totalDuration > 0.0f) {
          time_to_use = fmod(elapsed, animation->totalDuration);
        }

        int current_frame = 0;
        for (int i = 0; i < static_cast<int>(animation->frames.size()); ++i) {
          if (time_to_use >= animation->frames[i].before_time) {
            current_frame = i;
          }
          else {
            break;
          }
        }

        atlas_index = animation->frames[current_frame].atlas_index;
      }
      else {
        // Use live animation's current frame
        atlas_index = animation->get_current_atlas_index();
      }
    }
    else {
      // No animation, use drawIndex if valid, else 0
      if (texture_atlas->drawIndex >= 0) {
        atlas_index = texture_atlas->drawIndex;
      }
    }

    if (atlas_index >= 0 && atlas_index < static_cast<int>(texture_atlas->subTextures.size())) {
      const auto& subTex = texture_atlas->subTextures[atlas_index];
      // tex_coords: 0=BL, 1=BR, 2=TR, 3=TL
      // ImGui wants TL (uv0) and BR (uv1)
      uv0 = { subTex.tex_coords[3].x, subTex.tex_coords[3].y };
      uv1 = { subTex.tex_coords[1].x, subTex.tex_coords[1].y };

      // Note: We do NOT adjust sprite_world_width/height based on UVs here
      // because the User said "scale is how many pixel it is on the screen".
      // The Transform2D scale defines the FINAL size of the rendered quad,
      // regardless of how much texture it displays.
    }
  }

  // 1. Calculate available space
  ImVec2 avail_size = ImGui::GetContentRegionAvail();
  if (avail_size.x < 100 || avail_size.y < 100) avail_size = { 400, 400 };

  // 2. (Sprite dimensions calculated above as sprite_world_width/height)

  // 3. Calculate scale to fit sprite in window while maintaining aspect ratio
  // Avoid division by zero
  if (sprite_world_width <= 0.001f) sprite_world_width = 1.0f;
  if (sprite_world_height <= 0.001f) sprite_world_height = 1.0f;

  float scale_x = avail_size.x / sprite_world_width;
  float scale_y = avail_size.y / sprite_world_height;
  float scale = std::min(scale_x, scale_y) * 0.8f; // 0.8 to leave some padding

  float draw_width = sprite_world_width * scale;
  float draw_height = sprite_world_height * scale;

  // 4. Center the image
  ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
  float offset_x = (avail_size.x - draw_width) * 0.5f;
  float offset_y = (avail_size.y - draw_height) * 0.5f;

  ImVec2 image_min = { cursor_pos.x + offset_x, cursor_pos.y + offset_y };
  ImVec2 image_max = { image_min.x + draw_width, image_min.y + draw_height };

  // Draw the sprite
  ImGui::SetCursorScreenPos(image_min);
  ImGui::Image((ImTextureID)(intptr_t)texture_handle->id, { draw_width, draw_height }, uv0, uv1);

  // 5. Draw Collider and Handle Interaction
  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  // Center of the drawn image (Sprite Center)
  ImVec2 center = { image_min.x + draw_width * 0.5f, image_min.y + draw_height * 0.5f };

  // Apply flip to offset ONLY in live mode
  Math::Vec2D display_offset = col_offset;
  if (should_apply_flip) {
    if (auto* flip = entity.get_component<Components::Flip>()) {
      display_offset.x *= flip->value.x;
      display_offset.y *= flip->value.y;
    }
  }

  float handle_size = 8.0f;
  ImU32 handle_color = IM_COL32(0, 255, 0, 255);
  ImVec2 delta;
  bool changed = false;

  // Sync current_type_index with loaded collider type only when entity/state changes
  // Don't overwrite user's combo box selection
  if (!type_just_changed) {
    current_type_index = (col_type == Components::ColliderType::POLYGON) ? 1 : 0;
  }
  type_just_changed = false;

  if (current_type_index == 0) {
    // ===== AABB MODE =====
    // Collider Center (Sprite Center + Display Offset)
    ImVec2 col_center = { center.x + display_offset.x * scale, center.y + display_offset.y * scale };

    // Collider half-extents scaled to screen
    float col_half_w = (col_width / 2.0f) * scale;
    float col_half_h = (col_height / 2.0f) * scale;

    // Collider screen coords
    ImVec2 col_min = { col_center.x - col_half_w, col_center.y - col_half_h };
    ImVec2 col_max = { col_center.x + col_half_w, col_center.y + col_half_h };

    // Draw Rect
    draw_list->AddRect(col_min, col_max, IM_COL32(0, 255, 0, 255), 0.0f, 0, 2.0f);

    // --- Body Drag (Offset) ---
    ImGui::SetCursorScreenPos(col_min);
    ImGui::PushID("##collider_body_drag");
    ImGui::InvisibleButton("##body", { col_max.x - col_min.x, col_max.y - col_min.y });
    ImGui::PopID();

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      delta = ImGui::GetIO().MouseDelta;

      float flip_x = 1.0f;
      float flip_y = 1.0f;
      if (should_apply_flip) {
        if (auto* flip = entity.get_component<Components::Flip>()) {
          flip_x = flip->value.x;
          flip_y = flip->value.y;
        }
      }

      col_offset.x += (delta.x / scale) * flip_x;
      col_offset.y += (delta.y / scale) * flip_y;
      changed = true;
    }

    if (ImGui::IsItemHovered()) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }

    // Edge handles
    if (draw_handle(draw_list, { col_max.x, col_center.y }, handle_size, handle_color, 0, delta)) {
      col_width += (delta.x / scale) * 2.0f;
      if (col_width < 0) col_width = 0;
      changed = true;
    }

    if (draw_handle(draw_list, { col_min.x, col_center.y }, handle_size, handle_color, 1, delta)) {
      col_width -= (delta.x / scale) * 2.0f;
      if (col_width < 0) col_width = 0;
      changed = true;
    }

    if (draw_handle(draw_list, { col_center.x, col_min.y }, handle_size, handle_color, 2, delta)) {
      col_height -= (delta.y / scale) * 2.0f;
      if (col_height < 0) col_height = 0;
      changed = true;
    }

    if (draw_handle(draw_list, { col_center.x, col_max.y }, handle_size, handle_color, 3, delta)) {
      col_height += (delta.y / scale) * 2.0f;
      if (col_height < 0) col_height = 0;
      changed = true;
    }
  }
  else {
    // ===== POLYGON MODE =====
    // Draw polygon edges
    if (col_vertices.size() >= 2) {
      for (size_t i = 0; i < col_vertices.size(); ++i) {
        size_t next = (i + 1) % col_vertices.size();

        // Apply offset and scale to vertices
        ImVec2 v1 = { center.x + (col_vertices[i].x + display_offset.x) * scale,
                      center.y + (col_vertices[i].y + display_offset.y) * scale };
        ImVec2 v2 = { center.x + (col_vertices[next].x + display_offset.x) * scale,
                      center.y + (col_vertices[next].y + display_offset.y) * scale };

        draw_list->AddLine(v1, v2, IM_COL32(0, 255, 0, 255), 2.0f);
      }
    }

    // Draw and interact with vertices
    static int selected_vertex = -1;
    static int hovered_vertex = -1;
    hovered_vertex = -1;

    for (size_t i = 0; i < col_vertices.size(); ++i) {
      ImVec2 vertex_screen = { center.x + (col_vertices[i].x + display_offset.x) * scale,
                                center.y + (col_vertices[i].y + display_offset.y) * scale };

      ImU32 vertex_color = (selected_vertex == static_cast<int>(i)) ? IM_COL32(255, 255, 0, 255) : IM_COL32(0, 255, 0, 255);

      if (draw_handle(draw_list, vertex_screen, handle_size, vertex_color, 100 + static_cast<int>(i), delta)) {
        // Drag vertex
        col_vertices[i].x += delta.x / scale;
        col_vertices[i].y += delta.y / scale;
        changed = true;
        selected_vertex = static_cast<int>(i);
      }

      // Check if mouse is hovering this vertex
      ImVec2 mouse_pos = ImGui::GetMousePos();
      float dist_sq = (mouse_pos.x - vertex_screen.x) * (mouse_pos.x - vertex_screen.x) +
        (mouse_pos.y - vertex_screen.y) * (mouse_pos.y - vertex_screen.y);
      if (dist_sq < handle_size * handle_size) {
        hovered_vertex = static_cast<int>(i);
      }
    }

    // Right-click to delete vertex
    if (hovered_vertex >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
      if (col_vertices.size() > 3) { // Keep at least 3 vertices
        col_vertices.erase(col_vertices.begin() + hovered_vertex);
        changed = true;
        selected_vertex = -1;
      }
    }

    // Click on sprite (but not on a vertex) to add new vertex
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      ImVec2 mouse_pos = ImGui::GetMousePos();

      // Check if mouse is within the sprite bounds and not on a vertex
      if (mouse_pos.x >= image_min.x && mouse_pos.x <= image_max.x &&
        mouse_pos.y >= image_min.y && mouse_pos.y <= image_max.y &&
        hovered_vertex == -1 && selected_vertex == -1) {

        // Convert screen position to local vertex position  
        Math::Vec2D new_vertex;
        new_vertex.x = (mouse_pos.x - center.x) / scale - display_offset.x;
        new_vertex.y = (mouse_pos.y - center.y) / scale - display_offset.y;

        col_vertices.push_back(new_vertex);
        changed = true;
      }
    }

    // Show vertex count
    ImGui::SetCursorScreenPos({ image_min.x, image_min.y - 20 });
    ImGui::Text("Vertices: %zu (Click to add, Drag to move, Right-click to delete)", col_vertices.size());
  }

  // Apply changes
  // Also handle type changes from dropdown
  Components::ColliderType new_type = (current_type_index == 1) ? Components::ColliderType::POLYGON : Components::ColliderType::AABB;
  if (new_type != col_type) {
    col_type = new_type;
    changed = true;

    // When switching to polygon, initialize with a box if no vertices
    if (col_type == Components::ColliderType::POLYGON && col_vertices.empty()) {
      float hw = col_width / 2.0f;
      float hh = col_height / 2.0f;
      col_vertices = {
        Math::Vec2D(-hw, -hh),
        Math::Vec2D(-hw, hh),
        Math::Vec2D(hw, hh),
        Math::Vec2D(hw, -hh)
      };
    }
  }

  if (changed) {
    if (editing_specific_state) {
      auto& state = state_machine->states[current_editing_state];
      if (state.animation.has_value()) {
        // Ensure collider data exists
        if (!state.animation->collider_data.has_value()) {
          state.animation->collider_data = Components::ColliderData();
          state.animation->collider_data->type = col_type;
          state.animation->collider_data->width = col_width;
          state.animation->collider_data->height = col_height;
          state.animation->collider_data->offset = col_offset;
          state.animation->collider_data->vertices = col_vertices;
        }
        else {
          state.animation->collider_data->type = col_type;
          state.animation->collider_data->width = col_width;
          state.animation->collider_data->height = col_height;
          state.animation->collider_data->offset = col_offset;
          state.animation->collider_data->vertices = col_vertices;
        }
      }
    }
    else {
      // Live update
      collider->type = col_type;
      collider->width = col_width;
      collider->height = col_height;
      collider->offset = col_offset;
      collider->aabb.half_width = col_width / 2.0f;
      collider->aabb.half_height = col_height / 2.0f;

      // Update polygon vertices
      if (col_type == Components::ColliderType::POLYGON) {
        collider->polygon.vertices = col_vertices;
      }
    }
  }

  // Display values
  ImGui::SetCursorScreenPos({ image_min.x, image_max.y + 10 });
  ImGui::Text("Collider Size: %.2f x %.2f", col_width, col_height);
  ImGui::Text("Collider Offset: %.2f, %.2f", col_offset.x, col_offset.y);

  ImGui::SameLine();
  if (ImGui::Button("Reset to Sprite Size")) {
    col_width = sprite_world_width;
    col_height = sprite_world_height;
    col_offset = { 0.0f, 0.0f };
    changed = true;

    // Apply reset immediately
    if (editing_specific_state) {
      auto& state = state_machine->states[current_editing_state];
      if (state.animation.has_value()) {
        Components::ColliderData data;
        data.type = col_type;
        data.width = col_width;
        data.height = col_height;
        data.offset = col_offset;
        data.vertices = col_vertices;
        state.animation->collider_data = data;
      }
    }
    else {
      collider->width = col_width;
      collider->height = col_height;
      collider->offset = col_offset;
      collider->aabb.half_width = col_width / 2.0f;
      collider->aabb.half_height = col_height / 2.0f;
    }
  }

  if (editing_specific_state) {
    auto& state = state_machine->states[current_editing_state];
    if (state.animation.has_value() && state.animation->collider_data.has_value()) {
      ImGui::SameLine();
      if (ImGui::Button("Remove Override")) {
        state.animation->collider_data.reset();
      }
    }
  }

  ImGui::End();
}
