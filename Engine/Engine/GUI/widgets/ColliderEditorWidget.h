//-----------------------------------------------------------------------------------
// file:    ColliderEditorWidget.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Reusable ImGui widget for visual collider editing. Supports both AABB
//          and Polygon collider types with interactive handles for resizing and
//          vertex manipulation. Shared by ColliderEditorPanel and TileColliderEditorPanel.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../pch.h"
#include "../../Math/Vector2D.h"
#include "../../Graphics/Renderer.h"
#include <vector>

namespace ImGuiWidgets {

  /*=================================================================================
  Helper Function:
  Draws an interactive handle at the specified position and returns true if dragged.

  Parameters:
  draw_list  - ImGui draw list for rendering
  pos        - Screen position to draw the handle
  size       - Size of the handle in pixels
  color      - Color of the handle
  id         - Unique ID for this handle
  out_delta  - [out] Mouse delta if handle is being dragged

  Returns:
  true if the handle is being dragged, false otherwise.
  ==================================================================================*/
  bool ColliderHandle(
    ImDrawList* draw_list,
    ImVec2 pos,
    float size,
    ImU32 color,
    int id,
    ImVec2& out_delta
  );

  /*=================================================================================
  Structure to hold AABB collider editing state and configuration.
  ==================================================================================*/
  struct AABBColliderData {
    float width = 0.0f;
    float height = 0.0f;
    Math::Vec2D offset = { 0.0f, 0.0f };
  };

  /*=================================================================================
  Structure to hold Polygon collider editing state and configuration.
  ==================================================================================*/
  struct PolygonColliderData {
    std::vector<Math::Vec2D> vertices;
  };

  /*=================================================================================
  Function Brief:
  Renders an interactive AABB collider editor overlay on top of a sprite.

  Parameters:
  draw_list           - ImGui draw list for rendering
  center              - Screen position of the sprite center
  scale               - Scale factor from world units to screen pixels
  collider_data       - [in/out] AABB collider data to edit
  should_apply_flip   - Whether to apply flip to offset display
  flip_x              - X flip multiplier (1.0 or -1.0)
  flip_y              - Y flip multiplier (1.0 or -1.0)

  Returns:
  true if the collider was modified, false otherwise.

  Where to call it:
  After drawing the sprite in the visual editor, before displaying values.
  ==================================================================================*/
  bool AABBColliderEditor(
    ImDrawList* draw_list,
    ImVec2 center,
    float scale,
    AABBColliderData& collider_data,
    bool should_apply_flip = false,
    float flip_x = 1.0f,
    float flip_y = 1.0f
  );

  /*=================================================================================
  Function Brief:
  Renders an interactive Polygon collider editor overlay on top of a sprite.

  Parameters:
  draw_list       - ImGui draw list for rendering
  center          - Screen position of the sprite center
  scale           - Scale factor from world units to screen pixels
  collider_data   - [in/out] Polygon collider data to edit
  display_offset  - Offset to apply for display purposes (already flipped if needed)
  image_min       - Screen position of the top-left corner of the sprite
  image_max       - Screen position of the bottom-right corner of the sprite

  Returns:
  true if the collider was modified, false otherwise.

  Where to call it:
  After drawing the sprite in the visual editor, before displaying values.
  ==================================================================================*/
  bool PolygonColliderEditor(
    ImDrawList* draw_list,
    ImVec2 center,
    float scale,
    PolygonColliderData& collider_data,
    Math::Vec2D display_offset = { 0.0f, 0.0f },
    ImVec2 image_min = { 0, 0 },
    ImVec2 image_max = { 0, 0 }
  );

  /*=================================================================================
  Structure to hold sprite rendering configuration for the visual editor.
  ==================================================================================*/
  struct SpriteRenderConfig {
    Engine::Graphics::Texture texture_handle = nullptr;
    ImVec2 uv0 = { 0, 0 };
    ImVec2 uv1 = { 1, 1 };
    float sprite_world_width = 64.0f;
    float sprite_world_height = 64.0f;
  };

  /*=================================================================================
  Structure to hold calculated visual editor layout information.
  ==================================================================================*/
  struct VisualEditorLayout {
    float scale = 1.0f;
    float draw_width = 64.0f;
    float draw_height = 64.0f;
    ImVec2 image_min = { 0, 0 };
    ImVec2 image_max = { 0, 0 };
    ImVec2 center = { 0, 0 };
  };

  /*=================================================================================
  Function Brief:
  Calculates the layout for the visual editor (scale, positions, etc.) and returns
  the layout information. Does NOT draw anything.

  Parameters:
  sprite_config - Sprite rendering configuration
  avail_size    - Available ImGui content region size (optional)
  padding       - Padding multiplier (0.8 = 20% padding, 1.0 = no padding)

  Returns:
  VisualEditorLayout with calculated scale, positions, and dimensions.

  Where to call it:
  Before drawing the sprite and collider overlay.
  ==================================================================================*/
  VisualEditorLayout CalculateVisualEditorLayout(
    const SpriteRenderConfig& sprite_config,
    ImVec2 avail_size = { 0, 0 },
    float padding = 0.8f
  );

  /*=================================================================================
  Function Brief:
  Draws the sprite at the calculated layout position.

  Parameters:
  sprite_config - Sprite rendering configuration
  layout        - Pre-calculated layout from CalculateVisualEditorLayout

  Where to call it:
  After CalculateVisualEditorLayout, before drawing collider overlay.
  ==================================================================================*/
  void DrawSprite(
    const SpriteRenderConfig& sprite_config,
    const VisualEditorLayout& layout
  );

} // namespace ImGuiWidgets
