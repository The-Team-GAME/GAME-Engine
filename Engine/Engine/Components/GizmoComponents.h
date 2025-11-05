#pragma once

#include "../Math/Vector2D.h"

namespace Components
{
  struct Gizmo
  {
    bool is_dragging = false;
    Math::Vec2D drag_offset; // Offset from entity position to mouse click position
    Math::Vec4D outline_color = { 0.0f, 1.0f, 0.0f, 1.0f }; // Green outline by default
    float outline_thickness = 2.0f;

    Gizmo() = default;
    Gizmo(const Math::Vec4D& color, float thickness = 2.0f)
      : outline_color(color), outline_thickness(thickness) {
    }
  };
}

//REGISTER_COMPONENT(Components::Gizmo, is_dragging, drag_offset, outline_color, outline_thickness)
