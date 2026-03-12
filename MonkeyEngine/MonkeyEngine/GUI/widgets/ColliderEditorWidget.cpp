//-----------------------------------------------------------------------------------
// file:    ColliderEditorWidget.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of reusable collider editing widgets. Provides interactive
//          visual editing for AABB and Polygon colliders with handle-based resizing
//          and vertex manipulation.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ColliderEditorWidget.h"
#include <algorithm>
#include <limits>

namespace ImGuiWidgets {

  bool ColliderHandle(
    ImDrawList* draw_list,
    ImVec2 pos,
    float size,
    ImU32 color,
    int id,
    ImVec2& out_delta
  ) {
    ImVec2 min = { pos.x - size / 2, pos.y - size / 2 };
    ImVec2 max = { pos.x + size / 2, pos.y + size / 2 };

    draw_list->AddRectFilled(min, max, color);
    draw_list->AddRect(min, max, IM_COL32(0, 0, 0, 255));

    ImGui::SetCursorScreenPos(min);
    ImGui::PushID(id);
    ImGui::InvisibleButton("##handle", { size, size });
    ImGui::PopID();

    // bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();

    if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      ImVec2 delta = ImGui::GetIO().MouseDelta;
      out_delta = delta;
      return true;
    }
    return false;
  }

  bool AABBColliderEditor(
    ImDrawList* draw_list,
    ImVec2 center,
    float scale,
    AABBColliderData& collider_data,
    bool should_apply_flip,
    float flip_x,
    float flip_y
  ) {
    // Ensure minimum collider size to prevent degenerate cases
    const float min_size = 1.0f;
    if (collider_data.width < min_size) collider_data.width = min_size;
    if (collider_data.height < min_size) collider_data.height = min_size;

    // Convert collider data (center-based) to UI box (min/max corners in world space)
    // UI box is relative to sprite center (not affected by flip yet)
    float ui_min_x = collider_data.offset.x - collider_data.width / 2.0f;
    float ui_min_y = collider_data.offset.y - collider_data.height / 2.0f;
    float ui_max_x = collider_data.offset.x + collider_data.width / 2.0f;
    float ui_max_y = collider_data.offset.y + collider_data.height / 2.0f;

    // Apply flip for display
    Math::Vec2D display_min = { ui_min_x, ui_min_y };
    Math::Vec2D display_max = { ui_max_x, ui_max_y };
    if (should_apply_flip) {
      display_min.x *= flip_x;
      display_min.y *= flip_y;
      display_max.x *= flip_x;
      display_max.y *= flip_y;

      // Ensure min is still less than max after flip
      if (display_min.x > display_max.x) std::swap(display_min.x, display_max.x);
      if (display_min.y > display_max.y) std::swap(display_min.y, display_max.y);
    }

    // Convert to screen space
    ImVec2 col_min = { center.x + display_min.x * scale, center.y + display_min.y * scale };
    ImVec2 col_max = { center.x + display_max.x * scale, center.y + display_max.y * scale };
    ImVec2 col_center = { (col_min.x + col_max.x) * 0.5f, (col_min.y + col_max.y) * 0.5f };

    // Draw Rect
    draw_list->AddRect(col_min, col_max, IM_COL32(0, 255, 0, 255), 0.0f, 0, 2.0f);

    float handle_size = 8.0f;
    ImU32 handle_color = IM_COL32(0, 255, 0, 255);
    ImVec2 delta;
    bool changed = false;

    // --- Body Drag (Offset) ---
    ImGui::SetCursorScreenPos(col_min);
    ImGui::PushID("##collider_body_drag");
    ImGui::InvisibleButton("##body", { col_max.x - col_min.x, col_max.y - col_min.y });
    ImGui::PopID();

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      delta = ImGui::GetIO().MouseDelta;

      float actual_flip_x = should_apply_flip ? flip_x : 1.0f;
      float actual_flip_y = should_apply_flip ? flip_y : 1.0f;

      collider_data.offset.x += (delta.x / scale) * actual_flip_x;
      collider_data.offset.y += (delta.y / scale) * actual_flip_y;
      changed = true;
    }

    if (ImGui::IsItemHovered()) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }

    // Edge and corner handles
    bool shift_held = ImGui::GetIO().KeyShift;

    // Helper lambda to convert UI box back to collider data
    auto update_collider_from_ui_box = [&](float new_ui_min_x, float new_ui_min_y, float new_ui_max_x, float new_ui_max_y) {
      // Ensure min < max
      if (new_ui_min_x > new_ui_max_x) std::swap(new_ui_min_x, new_ui_max_x);
      if (new_ui_min_y > new_ui_max_y) std::swap(new_ui_min_y, new_ui_max_y);

      // Calculate new width, height, and offset
      float new_width = new_ui_max_x - new_ui_min_x;
      float new_height = new_ui_max_y - new_ui_min_y;

      // Enforce minimum size
      if (new_width < min_size) new_width = min_size;
      if (new_height < min_size) new_height = min_size;

      // Update collider data
      collider_data.width = new_width;
      collider_data.height = new_height;
      collider_data.offset.x = (new_ui_min_x + new_ui_max_x) * 0.5f;
      collider_data.offset.y = (new_ui_min_y + new_ui_max_y) * 0.5f;
      };

    // Right handle
    if (ColliderHandle(draw_list, { col_max.x, col_center.y }, handle_size, handle_color, 0, delta)) {
      float dx = delta.x / scale;
      if (shift_held) {
        // Shift: scale symmetrically from center
        update_collider_from_ui_box(ui_min_x - dx, ui_min_y, ui_max_x + dx, ui_max_y);
      }
      else {
        // No shift: move right edge only
        update_collider_from_ui_box(ui_min_x, ui_min_y, ui_max_x + dx, ui_max_y);
      }
      changed = true;
    }

    // Left handle
    if (ColliderHandle(draw_list, { col_min.x, col_center.y }, handle_size, handle_color, 1, delta)) {
      float dx = delta.x / scale;
      if (shift_held) {
        // Shift: scale symmetrically from center
        update_collider_from_ui_box(ui_min_x - dx, ui_min_y, ui_max_x + dx, ui_max_y);
      }
      else {
        // No shift: move left edge only
        update_collider_from_ui_box(ui_min_x + dx, ui_min_y, ui_max_x, ui_max_y);
      }
      changed = true;
    }

    // Top handle
    if (ColliderHandle(draw_list, { col_center.x, col_min.y }, handle_size, handle_color, 2, delta)) {
      float dy = delta.y / scale;
      if (shift_held) {
        // Shift: scale symmetrically from center
        update_collider_from_ui_box(ui_min_x, ui_min_y - dy, ui_max_x, ui_max_y + dy);
      }
      else {
        // No shift: move top edge only
        update_collider_from_ui_box(ui_min_x, ui_min_y + dy, ui_max_x, ui_max_y);
      }
      changed = true;
    }

    // Bottom handle
    if (ColliderHandle(draw_list, { col_center.x, col_max.y }, handle_size, handle_color, 3, delta)) {
      float dy = delta.y / scale;
      if (shift_held) {
        // Shift: scale symmetrically from center
        update_collider_from_ui_box(ui_min_x, ui_min_y - dy, ui_max_x, ui_max_y + dy);
      }
      else {
        // No shift: move bottom edge only
        update_collider_from_ui_box(ui_min_x, ui_min_y, ui_max_x, ui_max_y + dy);
      }
      changed = true;
    }

    // Corner handles for diagonal scaling
    ImU32 corner_handle_color = IM_COL32(255, 255, 0, 255); // Yellow for corners

    // Top-left corner
    if (ColliderHandle(draw_list, col_min, handle_size, corner_handle_color, 4, delta)) {
      float dx = delta.x / scale;
      float dy = delta.y / scale;
      if (shift_held) {
        // Shift: scale symmetrically from center
        update_collider_from_ui_box(ui_min_x - dx, ui_min_y - dy, ui_max_x + dx, ui_max_y + dy);
      }
      else {
        // No shift: move top-left corner only
        update_collider_from_ui_box(ui_min_x + dx, ui_min_y + dy, ui_max_x, ui_max_y);
      }
      changed = true;
    }

    // Top-right corner
    if (ColliderHandle(draw_list, { col_max.x, col_min.y }, handle_size, corner_handle_color, 5, delta)) {
      float dx = delta.x / scale;
      float dy = delta.y / scale;
      if (shift_held) {
        // Shift: scale symmetrically from center
        update_collider_from_ui_box(ui_min_x - dx, ui_min_y - dy, ui_max_x + dx, ui_max_y + dy);
      }
      else {
        // No shift: move top-right corner only
        update_collider_from_ui_box(ui_min_x, ui_min_y + dy, ui_max_x + dx, ui_max_y);
      }
      changed = true;
    }

    // Bottom-left corner
    if (ColliderHandle(draw_list, { col_min.x, col_max.y }, handle_size, corner_handle_color, 6, delta)) {
      float dx = delta.x / scale;
      float dy = delta.y / scale;
      if (shift_held) {
        // Shift: scale symmetrically from center
        update_collider_from_ui_box(ui_min_x - dx, ui_min_y - dy, ui_max_x + dx, ui_max_y + dy);
      }
      else {
        // No shift: move bottom-left corner only
        update_collider_from_ui_box(ui_min_x + dx, ui_min_y, ui_max_x, ui_max_y + dy);
      }
      changed = true;
    }

    // Bottom-right corner
    if (ColliderHandle(draw_list, col_max, handle_size, corner_handle_color, 7, delta)) {
      float dx = delta.x / scale;
      float dy = delta.y / scale;
      if (shift_held) {
        // Shift: scale symmetrically from center
        update_collider_from_ui_box(ui_min_x - dx, ui_min_y - dy, ui_max_x + dx, ui_max_y + dy);
      }
      else {
        // No shift: move bottom-right corner only
        update_collider_from_ui_box(ui_min_x, ui_min_y, ui_max_x + dx, ui_max_y + dy);
      }
      changed = true;
    }

    return changed;
  }

  bool PolygonColliderEditor(
    ImDrawList* draw_list,
    ImVec2 center,
    float scale,
    PolygonColliderData& collider_data,
    Math::Vec2D display_offset,
    ImVec2 image_min,
    ImVec2 image_max
  ) {
    float handle_size = 8.0f;
    ImVec2 delta;
    bool changed = false;

    // Draw polygon edges
    if (collider_data.vertices.size() >= 2) {
      for (size_t i = 0; i < collider_data.vertices.size(); ++i) {
        size_t next = (i + 1) % collider_data.vertices.size();

        // Apply offset and scale to vertices
        ImVec2 v1 = { center.x + (collider_data.vertices[i].x + display_offset.x) * scale,
                      center.y + (collider_data.vertices[i].y + display_offset.y) * scale };
        ImVec2 v2 = { center.x + (collider_data.vertices[next].x + display_offset.x) * scale,
                      center.y + (collider_data.vertices[next].y + display_offset.y) * scale };

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

    for (size_t i = 0; i < collider_data.vertices.size(); ++i) {
      ImVec2 vertex_screen = { center.x + (collider_data.vertices[i].x + display_offset.x) * scale,
                                center.y + (collider_data.vertices[i].y + display_offset.y) * scale };

      ImU32 vertex_color = (selected_vertex == static_cast<int>(i)) ? IM_COL32(255, 255, 0, 255) : IM_COL32(0, 255, 0, 255);

      if (ColliderHandle(draw_list, vertex_screen, handle_size, vertex_color, 100 + static_cast<int>(i), delta)) {
        // Drag vertex
        collider_data.vertices[i].x += delta.x / scale;
        collider_data.vertices[i].y += delta.y / scale;
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

    // Only process click interactions when this window is focused and hovered
    bool window_can_receive_clicks = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
      ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    // Right-click to delete vertex
    if (window_can_receive_clicks && hovered_vertex >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
      if (collider_data.vertices.size() > 3) { // Keep at least 3 vertices
        collider_data.vertices.erase(collider_data.vertices.begin() + hovered_vertex);
        changed = true;
        selected_vertex = -1;
      }
    }

    // Click on sprite (but not on a vertex) to add new vertex
    if (window_can_receive_clicks && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered_vertex == -1) {
      ImVec2 mouse_pos = ImGui::GetMousePos();

      // Check if mouse is within the sprite bounds (if bounds provided)
      bool within_bounds = true;
      if (image_min.x != 0 || image_min.y != 0 || image_max.x != 0 || image_max.y != 0) {
        within_bounds = (mouse_pos.x >= image_min.x && mouse_pos.x <= image_max.x &&
          mouse_pos.y >= image_min.y && mouse_pos.y <= image_max.y);
      }

      if (within_bounds) {
        // Convert screen position to local vertex position  
        Math::Vec2D new_vertex;
        new_vertex.x = (mouse_pos.x - center.x) / scale - display_offset.x;
        new_vertex.y = (mouse_pos.y - center.y) / scale - display_offset.y;

        // Find the closest edge and insert the vertex there
        if (collider_data.vertices.size() >= 2) {
          float min_distance = std::numeric_limits<float>::max();
          size_t insert_after_index = 0;

          for (size_t i = 0; i < collider_data.vertices.size(); ++i) {
            size_t next = (i + 1) % collider_data.vertices.size();

            // Project mouse point onto the line segment
            Math::Vec2D v1 = collider_data.vertices[i];
            Math::Vec2D v2 = collider_data.vertices[next];
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
          collider_data.vertices.insert(collider_data.vertices.begin() + insert_after_index + 1, new_vertex);
        }
        else {
          // If less than 2 vertices, just append
          collider_data.vertices.push_back(new_vertex);
        }

        changed = true;
      }
    }

    // Show vertex count if vertices exist
    if (collider_data.vertices.size() > 0 && image_min.x != 0) {
      ImGui::SetCursorScreenPos({ image_min.x, image_min.y - 20 });
      ImGui::Text("Vertices: %zu (Click to add, Drag to move, Right-click to delete)", collider_data.vertices.size());
    }

    return changed;
  }

  VisualEditorLayout CalculateVisualEditorLayout(
    const SpriteRenderConfig& sprite_config,
    ImVec2 avail_size,
    float padding
  ) {
    VisualEditorLayout layout;

    // Get available space if not provided
    if (avail_size.x == 0 || avail_size.y == 0) {
      avail_size = ImGui::GetContentRegionAvail();
      if (avail_size.x < 100 || avail_size.y < 100) {
        avail_size = { 400, 400 };
      }
    }

    // Calculate scale to fit sprite in window while maintaining aspect ratio
    float sprite_world_width = sprite_config.sprite_world_width;
    float sprite_world_height = sprite_config.sprite_world_height;

    // Avoid division by zero
    if (sprite_world_width <= 0.001f) sprite_world_width = 1.0f;
    if (sprite_world_height <= 0.001f) sprite_world_height = 1.0f;

    float scale_x = avail_size.x / sprite_world_width;
    float scale_y = avail_size.y / sprite_world_height;
    layout.scale = std::min(scale_x, scale_y) * padding;

    layout.draw_width = sprite_world_width * layout.scale;
    layout.draw_height = sprite_world_height * layout.scale;

    // Center the image
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    float offset_x = (avail_size.x - layout.draw_width) * 0.5f;
    float offset_y = (avail_size.y - layout.draw_height) * 0.5f;

    layout.image_min = { cursor_pos.x + offset_x, cursor_pos.y + offset_y };
    layout.image_max = { layout.image_min.x + layout.draw_width, layout.image_min.y + layout.draw_height };
    layout.center = { layout.image_min.x + layout.draw_width * 0.5f, layout.image_min.y + layout.draw_height * 0.5f };

    return layout;
  }

  void DrawSprite(
    const SpriteRenderConfig& sprite_config,
    const VisualEditorLayout& layout
  ) {
    if (!sprite_config.texture_handle || sprite_config.texture_handle->id == 0) {
      return;
    }

    ImGui::SetCursorScreenPos(layout.image_min);
    ImGui::Image(
      (ImTextureID)(intptr_t)sprite_config.texture_handle->id,
      { layout.draw_width, layout.draw_height },
      sprite_config.uv0,
      sprite_config.uv1
    );
  }

} // namespace ImGuiWidgets
