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
#include "../ECS/Prefab.h"
#include "../Components/CollisionComponents.h"
#include "../Components/StateMachineComponents.h"
#include "../Components/GraphicsComponents.h"
#include "../Components/SpatialComponents.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include "../Graphics/Renderer.h" // For Texture
#include "../InputManager.h"
#include "../ProjectSaveManager.h" // For marking prefabs dirty
#include "../Resources.h"
#include "../Logger.h"

namespace {
  enum class EditorMode { None, Entity, Prefab };
  EditorMode current_mode = EditorMode::None;

  s32 selected_entity_id = -1;
  Engine::ECS::Prefab selected_prefab;
  std::string selected_prefab_path = "";
  bool is_entity_prefab_instance = false;
  std::string entity_source_prefab_path = "";

  bool selection_initialized = false;
  std::string preview_state_name = "";
  float preview_animation_start_time = 0.0f;

  void init_selection_listener() {
    if (selection_initialized) return;
    selection_initialized = true;

    Engine::EventManager::add_event_listener<Engine::Events::SelectEntityEvent>(
      [](const Engine::Events::SelectEntityEvent& event) {
        selected_entity_id = static_cast<s32>(event.entity_id);
        if (selected_entity_id != -1) {
          current_mode = EditorMode::Entity;

          // Check if this entity is a prefab instance
          Engine::ECS::Entity entity = Engine::ECS::get_entity(event.entity_id);
          is_entity_prefab_instance = Engine::ECS::is_prefab_instance(entity);
          if (is_entity_prefab_instance) {
            entity_source_prefab_path = Engine::ECS::get_prefab_path(entity);
          }
          else {
            entity_source_prefab_path = "";
          }
        }
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

namespace ColliderEditor {
  void Open(Engine::ECS::Entity entity) {
    current_mode = EditorMode::Entity;
    selected_entity_id = static_cast<s32>(entity.get_id());
    ImGui::SetWindowFocus("Collider Editor");
  }

  void Open(Engine::ECS::Prefab prefab, const std::string& prefab_path) {
    current_mode = EditorMode::Prefab;
    selected_prefab = prefab;
    selected_prefab_path = prefab_path;
    ImGui::SetWindowFocus("Collider Editor");
  }
}

void ColliderEditorPanel()
{
  init_selection_listener();

  ImGui::Begin("Collider Editor");

  if (current_mode == EditorMode::None) {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No entity or prefab selected");
    ImGui::End();
    return;
  }

  // --- Context Retrieval ---
  Components::BoxCollider* collider = nullptr;
  Components::Texture* texture = nullptr;
  Components::StateMachine* state_machine = nullptr;
  Components::TextureAtlas* texture_atlas = nullptr;
  Components::Animation* animation = nullptr;
  Components::Transform2D* transform = nullptr;
  Components::Flip* flip = nullptr;

  Engine::ECS::Entity entity; // Only valid in Entity mode

  if (current_mode == EditorMode::Entity) {
    if (selected_entity_id == -1) {
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No entity selected");
      ImGui::End();
      return;
    }
    entity = Engine::ECS::get_entity(selected_entity_id);
    if (!entity.is_valid()) {
      selected_entity_id = -1;
      current_mode = EditorMode::None;
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Selected entity no longer exists");
      ImGui::End();
      return;
    }

    collider = entity.get_component<Components::BoxCollider>();
    texture = entity.get_component<Components::Texture>();
    state_machine = entity.get_component<Components::StateMachine>();
    texture_atlas = entity.get_component<Components::TextureAtlas>();
    animation = entity.get_component<Components::Animation>();
    transform = entity.get_component<Components::Transform2D>();
    flip = entity.get_component<Components::Flip>();

    // Display prefab instance info if applicable
    if (is_entity_prefab_instance && !entity_source_prefab_path.empty()) {
      ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Prefab Instance: %s", entity_source_prefab_path.c_str());
      ImGui::SameLine();
      if (ImGui::Button("Edit Prefab Directly")) {
        // Load the source prefab and switch to prefab editing mode
        Engine::ECS::Prefab source_prefab = Engine::Resources::load_prefab(entity_source_prefab_path);
        if (source_prefab.is_valid()) {
          ColliderEditor::Open(source_prefab, entity_source_prefab_path);
        }
      }
      ImGui::Separator();
    }
  }
  else if (current_mode == EditorMode::Prefab) {
    if (!selected_prefab.is_valid()) {
      current_mode = EditorMode::None;
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Selected prefab is invalid");
      ImGui::End();
      return;
    }

    collider = selected_prefab.get_component<Components::BoxCollider>();
    texture = selected_prefab.get_component<Components::Texture>();
    state_machine = selected_prefab.get_component<Components::StateMachine>();
    texture_atlas = selected_prefab.get_component<Components::TextureAtlas>();
    animation = selected_prefab.get_component<Components::Animation>();
    transform = selected_prefab.get_component<Components::Transform2D>();
    flip = selected_prefab.get_component<Components::Flip>();

    ImGui::Text("Editing Prefab: %s", selected_prefab.get_impl()->get_prefab_id().c_str());
  }

  if (!collider) {
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "Selected object has no BoxCollider");
    if (ImGui::Button("Add BoxCollider")) {
      if (current_mode == EditorMode::Entity) {
        entity.add_component<Components::BoxCollider>(50.0f, 50.0f, false);
      }
      else {
        selected_prefab.add_component<Components::BoxCollider>(50.0f, 50.0f, false);
        // Mark dirty
        if (!selected_prefab_path.empty()) {
          Engine::ProjectSaveManager::set_dirty(
            Engine::ProjectSaveManager::DirtyStates::Prefab(selected_prefab_path)
          );
        }
        // We don't have the path here easily, but update_prefab_instances will be called
        Engine::ECS::update_prefab_instances(selected_prefab.get_impl()->get_prefab_id());
      }
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
    if (ImGui::BeginCombo("Edit State", selected_state_name.empty() ? "Current (Live/Default)" : selected_state_name.c_str())) {

      if (ImGui::Selectable("Current (Live/Default)", selected_state_name.empty())) {
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
  const Components::TextureAtlas* active_atlas = texture_atlas;
  const Components::Animation* active_animation = animation;

  float col_width = 0.0f;
  float col_height = 0.0f;
  Math::Vec2D col_offset = { 0.0f, 0.0f };
  Components::ColliderType col_type = Components::ColliderType::AABB;
  std::vector<Math::Vec2D> col_vertices; // For polygon colliders
  bool should_apply_flip = false; // Only apply flip in live mode (Entity)

  if (editing_specific_state) {
    // STATE EDITING MODE - completely isolated from live entity
    auto& state = state_machine->states[current_editing_state];

    // Get texture from state animation if available
    if (state.animation.has_value()) {
      if (state.animation->texture.texture_handle) {
        texture_handle = state.animation->texture.texture_handle;
        active_atlas = &state.animation->texture_atlas;
        active_animation = &state.animation->animation;

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
    // LIVE/DEFAULT EDITING MODE
    if (texture && texture->texture_handle) {
      texture_handle = texture->texture_handle;
    }

    col_type = collider->type;
    col_width = collider->width;
    col_height = collider->height;
    col_offset = collider->offset;
    col_vertices = collider->polygon.vertices;

    // Apply flip only if in Entity mode
    if (current_mode == EditorMode::Entity) {
      should_apply_flip = true;
    }
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
  if (transform) {
    sprite_world_width = transform->scale.x;
    sprite_world_height = transform->scale.y;
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
  if (active_atlas && !active_atlas->subTextures.empty()) {
    int atlas_index = 0;
    if (active_animation && !active_animation->frames.empty()) {
      if (editing_specific_state || current_mode == EditorMode::Prefab) {
        // Calculate animated frame using preview start time
        // Always loop in preview mode for better visualization
        float elapsed = Engine::Time::get_time() - preview_animation_start_time;
        float time_to_use = elapsed;

        if (active_animation->totalDuration > 0.0f) {
          time_to_use = fmod(elapsed, active_animation->totalDuration);
        }

        int current_frame = 0;
        for (int i = 0; i < static_cast<int>(active_animation->frames.size()); ++i) {
          if (time_to_use >= active_animation->frames[i].before_time) {
            current_frame = i;
          }
          else {
            break;
          }
        }

        atlas_index = active_animation->frames[current_frame].atlas_index;
      }
      else {
        // Use live animation's current frame (Entity mode only)
        atlas_index = active_animation->get_current_atlas_index();
      }
    }
    else {
      // No animation, use drawIndex if valid, else 0
      if (active_atlas->drawIndex >= 0) {
        atlas_index = active_atlas->drawIndex;
      }
    }

    if (atlas_index >= 0 && atlas_index < static_cast<int>(active_atlas->subTextures.size())) {
      const auto& subTex = active_atlas->subTextures[atlas_index];
      // tex_coords: 0=BL, 1=BR, 2=TR, 3=TL
      // ImGui wants TL (uv0) and BR (uv1)
      uv0 = { subTex.tex_coords[3].x, subTex.tex_coords[3].y };
      uv1 = { subTex.tex_coords[1].x, subTex.tex_coords[1].y };
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
  if (should_apply_flip && flip) {
    display_offset.x *= flip->value.x;
    display_offset.y *= flip->value.y;
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
      if (should_apply_flip && flip) {
        flip_x = flip->value.x;
        flip_y = flip->value.y;
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

    // Reset selection when not actively dragging
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      selected_vertex = -1;
    }

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
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered_vertex == -1) {
      ImVec2 mouse_pos = ImGui::GetMousePos();

      // Check if mouse is within the sprite bounds
      if (mouse_pos.x >= image_min.x && mouse_pos.x <= image_max.x &&
        mouse_pos.y >= image_min.y && mouse_pos.y <= image_max.y) {

        // Convert screen position to local vertex position  
        Math::Vec2D new_vertex;
        new_vertex.x = (mouse_pos.x - center.x) / scale - display_offset.x;
        new_vertex.y = (mouse_pos.y - center.y) / scale - display_offset.y;

        // Find the closest edge and insert the vertex there
        if (col_vertices.size() >= 2) {
          float min_distance = FLT_MAX;
          size_t insert_after_index = 0;

          for (size_t i = 0; i < col_vertices.size(); ++i) {
            size_t next = (i + 1) % col_vertices.size();

            // Project mouse point onto the line segment
            Math::Vec2D v1 = col_vertices[i];
            Math::Vec2D v2 = col_vertices[next];
            Math::Vec2D edge = v2 - v1;
            Math::Vec2D to_point = new_vertex - v1;

            float edge_length_sq = edge.x * edge.x + edge.y * edge.y;
            if (edge_length_sq < 0.0001f) continue; // Skip degenerate edges

            float t = (to_point.x * edge.x + to_point.y * edge.y) / edge_length_sq;
            t = std::max(0.0f, std::min(1.0f, t)); // Clamp to segment

            Math::Vec2D closest_point = v1 + edge * t;
            Math::Vec2D to_closest = new_vertex - closest_point;
            float distance = to_closest.x * to_closest.x + to_closest.y * to_closest.y;

            if (distance < min_distance) {
              min_distance = distance;
              insert_after_index = i;
            }
          }

          // Insert vertex after the closest edge
          col_vertices.insert(col_vertices.begin() + insert_after_index + 1, new_vertex);
        }
        else {
          // If less than 2 vertices, just append
          col_vertices.push_back(new_vertex);
        }

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
      // Live/Default update
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

      // If in Prefab mode, mark dirty
      if (current_mode == EditorMode::Prefab) {
        if (!selected_prefab_path.empty()) {
          Engine::ProjectSaveManager::set_dirty(
            Engine::ProjectSaveManager::DirtyStates::Prefab(selected_prefab_path)
          );
        }
        Engine::ECS::update_prefab_instances(selected_prefab.get_impl()->get_prefab_id());
      }
    }
  }

  // Display values
  ImGui::SetCursorScreenPos({ image_min.x, image_max.y + 10 });
  ImGui::Text("Collider Size: %.2f x %.2f", col_width, col_height);
  ImGui::Text("Collider Offset: %.2f, %.2f", col_offset.x, col_offset.y);

  // Show save to prefab button for prefab instances
  if (current_mode == EditorMode::Entity && is_entity_prefab_instance && !entity_source_prefab_path.empty() && !editing_specific_state) {
    if (ImGui::Button("Save Collider to Prefab")) {
      // Load the source prefab
      Engine::ECS::Prefab source_prefab = Engine::Resources::load_prefab(entity_source_prefab_path);
      if (source_prefab.is_valid()) {
        // Update the prefab's collider
        auto* prefab_collider = source_prefab.get_component<Components::BoxCollider>();
        if (prefab_collider) {
          prefab_collider->type = collider->type;
          prefab_collider->width = collider->width;
          prefab_collider->height = collider->height;
          prefab_collider->offset = collider->offset;
          prefab_collider->aabb.half_width = collider->width / 2.0f;
          prefab_collider->aabb.half_height = collider->height / 2.0f;
          prefab_collider->polygon.vertices = collider->polygon.vertices;

          // Mark the prefab as dirty to trigger save
          Engine::ProjectSaveManager::set_dirty(
            Engine::ProjectSaveManager::DirtyStates::Prefab(entity_source_prefab_path)
          );

          // Update all instances of this prefab
          Engine::ECS::update_prefab_instances(source_prefab.get_impl()->get_prefab_id());

          Log::info("Saved collider changes to prefab: " + entity_source_prefab_path);
        }
      }
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.0f, 1.0f), "(Changes only affect this instance)");
  }

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

      if (current_mode == EditorMode::Prefab) {
        if (!selected_prefab_path.empty()) {
          Engine::ProjectSaveManager::set_dirty(
            Engine::ProjectSaveManager::DirtyStates::Prefab(selected_prefab_path)
          );
        }
        Engine::ECS::update_prefab_instances(selected_prefab.get_impl()->get_prefab_id());
      }
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
