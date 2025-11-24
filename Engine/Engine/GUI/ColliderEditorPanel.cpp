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
#include "../Components/GraphicsComponents.h"
#include "../Components/SpatialComponents.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include "../Graphics/Renderer.h" // For Texture
#include "../InputManager.h"

namespace {
  s32 selected_entity_id = -1;
  bool selection_initialized = false;

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
  auto* transform = entity.get_component<Components::Transform2D>();

  if (!collider) {
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "Selected entity has no BoxCollider");
    if (ImGui::Button("Add BoxCollider")) {
       entity.add_component<Components::BoxCollider>(50.0f, 50.0f, false);
    }
    ImGui::End();
    return;
  }

  if (!texture || !texture->texture_handle) {
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "Selected entity has no Texture (cannot visualize)");
    // Fallback: just show numeric controls? Or maybe draw a placeholder box?
    // For now, let's just show the numeric controls which are already in EntityEditor, 
    // but we can duplicate them here for convenience.
    ImGui::DragFloat("Width", &collider->width);
    ImGui::DragFloat("Height", &collider->height);
    ImGui::End();
    return;
  }

  // --- Visualization Logic ---

  // 1. Calculate available space
  ImVec2 avail_size = ImGui::GetContentRegionAvail();
  if (avail_size.x < 100 || avail_size.y < 100) avail_size = { 400, 400 };

  // 2. Get sprite dimensions
  float sprite_width = static_cast<float>(texture->texture_handle->width);
  float sprite_height = static_cast<float>(texture->texture_handle->height);
  
  // 3. Calculate scale to fit sprite in window while maintaining aspect ratio
  float scale_x = avail_size.x / sprite_width;
  float scale_y = avail_size.y / sprite_height;
  float scale = std::min(scale_x, scale_y) * 0.8f; // 0.8 to leave some padding

  float draw_width = sprite_width * scale;
  float draw_height = sprite_height * scale;

  // 4. Center the image
  ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
  float offset_x = (avail_size.x - draw_width) * 0.5f;
  float offset_y = (avail_size.y - draw_height) * 0.5f;
  
  ImVec2 image_min = { cursor_pos.x + offset_x, cursor_pos.y + offset_y };
  ImVec2 image_max = { image_min.x + draw_width, image_min.y + draw_height };

  // Draw the sprite
  ImGui::SetCursorScreenPos(image_min);
  ImGui::Image((ImTextureID)(intptr_t)texture->texture_handle->id, { draw_width, draw_height });

  // 5. Draw Collider Rect
  // Collider dimensions are in world units. 
  // We need to map World Units -> Sprite Pixels -> Screen Pixels.
  // Assumption: The sprite's texture size corresponds to its world size when scale is 1.0?
  // Usually, 1 pixel = 1 world unit, or there's a PPU (Pixels Per Unit).
  // Let's assume 1-to-1 for now, or check Transform scale.
  // Actually, the BoxCollider width/height are in World Units.
  // The Texture width/height are in Pixels.
  // If the entity is rendered 1:1, then we can map directly.
  // However, the sprite might be scaled by Transform2D.scale.
  // BUT, we are viewing the "Prefab" / "Local" state here effectively, ignoring World Transform for the view?
  // Or are we trying to edit the collider relative to the sprite?
  // Yes, usually collider is defined relative to the entity center.
  
  // Let's assume the sprite image displayed represents the "unscaled" entity visual.
  // We need to know how many "World Units" the sprite image covers.
  // If we don't have PPU, we might have to guess or assume the texture size IS the world size (common in 2D engines).
  // Let's assume Texture Width (px) == World Width (units) for the base sprite.
  
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  
  // Center of the drawn image
  ImVec2 center = { image_min.x + draw_width * 0.5f, image_min.y + draw_height * 0.5f };

  // Collider half-extents scaled to screen
  float col_half_w = (collider->width / 2.0f) * scale;
  float col_half_h = (collider->height / 2.0f) * scale;

  // Collider screen coords
  ImVec2 col_min = { center.x - col_half_w, center.y - col_half_h };
  ImVec2 col_max = { center.x + col_half_w, center.y + col_half_h };

  // Draw Rect
  draw_list->AddRect(col_min, col_max, IM_COL32(0, 255, 0, 255), 0.0f, 0, 2.0f);

  // 6. Interaction Handles
  float handle_size = 8.0f;
  ImU32 handle_color = IM_COL32(0, 255, 0, 255);
  ImVec2 delta;

  // Right Edge
  if (draw_handle(draw_list, { col_max.x, center.y }, handle_size, handle_color, 0, delta)) {
    collider->width += (delta.x / scale) * 2.0f; // *2 because we are editing width from center
    if (collider->width < 0) collider->width = 0;
  }
  
  // Left Edge
  if (draw_handle(draw_list, { col_min.x, center.y }, handle_size, handle_color, 1, delta)) {
    collider->width -= (delta.x / scale) * 2.0f;
    if (collider->width < 0) collider->width = 0;
  }

  // Top Edge
  if (draw_handle(draw_list, { center.x, col_min.y }, handle_size, handle_color, 2, delta)) {
    collider->height -= (delta.y / scale) * 2.0f;
    if (collider->height < 0) collider->height = 0;
  }

  // Bottom Edge
  if (draw_handle(draw_list, { center.x, col_max.y }, handle_size, handle_color, 3, delta)) {
    collider->height += (delta.y / scale) * 2.0f;
    if (collider->height < 0) collider->height = 0;
  }

  // Sync AABB
  collider->aabb.half_width = collider->width / 2.0f;
  collider->aabb.half_height = collider->height / 2.0f;

  // Display values
  ImGui::SetCursorScreenPos({ image_min.x, image_max.y + 10 });
  ImGui::Text("Collider Size: %.2f x %.2f", collider->width, collider->height);
  ImGui::SameLine();
  if (ImGui::Button("Reset to Sprite Size")) {
    collider->width = sprite_width;
    collider->height = sprite_height;
    collider->aabb.half_width = sprite_width / 2.0f;
    collider->aabb.half_height = sprite_height / 2.0f;
  }

  ImGui::End();
}
