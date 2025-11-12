//-----------------------------------------------------------------------------------
// file:    GizmoComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for the gizmo system. Provides visual
//          feedback and interaction handling for selected entities in the
//          editor. Includes drag state, offset tracking, and outline styling
//          for entity manipulation in the viewport.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Math/Vector2D.h"

namespace Components
{

  enum class GizmoHandleType {
    None,
    Body,
    ScaleTL, ScaleTR, ScaleBR, ScaleBL,
    AxisX, // move-only along +X/-X
    AxisY  // move-only along +Y/-Y
  };

  struct Gizmo
  {
    // Interaction state
    bool is_dragging = false;
    GizmoHandleType active_handle = GizmoHandleType::None;
    GizmoHandleType hover_handle = GizmoHandleType::None;

    // Cached on press
    Math::Vec2D drag_offset;                   // Offset from entity position to mouse click position
    Math::Vec2D start_mouse_world;             // pressed mouse position
    Math::Vec2D start_pos;                     // original entity pos
    Math::Vec2D start_scale;                   // original entity scale
    Math::Vec2D scale_anchor;                  // world anchor point for scaling

    // Text scaling cache
    Math::Vec2D text_start_size{ 1.0f, 1.0f };

    Math::Vec4D outline_color = { 0.0f, 1.0f, 0.0f, 1.0f }; // Green outline by default
    float outline_thickness = 2.0f;

    // Handle visuals/geometry (world units)
    float handle_size = 10.0f;                 // square size of corner/axis handles
    Math::Vec4D handle_color = { 0.1f, 0.8f, 1.0f, 1.0f };
    Math::Vec4D handle_hover_color = { 1.0f, 0.9f, 0.1f, 1.0f };

    // Axis drag damping
    float axis_move_slow_factor = 0.25f;

    // Arrow visuals
    float arrow_length_factor = 0.6f;   // portion of rect edge
    float arrow_outside_extra = 18.0f;  // extra past the edge
    float arrow_head_size = 12.0f;      // head length
    float arrow_thickness = 3.0f;       // shaft thickness

    Gizmo() = default;
    Gizmo(const Math::Vec4D& color, float thickness = 2.0f)
      : outline_color(color), outline_thickness(thickness) {
    }
  };
}

//REGISTER_COMPONENT(Components::Gizmo, is_dragging, drag_offset, outline_color, outline_thickness)
