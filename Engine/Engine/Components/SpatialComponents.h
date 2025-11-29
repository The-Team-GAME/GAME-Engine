//-----------------------------------------------------------------------------------
// file:    SpatialComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for spatial transformations and positioning.
//          Includes Transform2D for position, rotation, and scale; ZIndex for
//          rendering order; and ScreenTransform2D for screen-space coordinates.
//          Provides the foundation for entity positioning and hierarchy.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Math/Vector2D.h"

namespace Components
{
  struct Transform2D
  {
    Math::Vec2D position;
    float rotation = 0.0f; // In degrees
    Math::Vec2D scale = { 1.0f, 1.0f };

    Transform2D() : position(0.0f, 0.0f), rotation(0.0f), scale(1.0f, 1.0f) {}
    Transform2D(const Math::Vec2D& pos, float rot = 0.0f, const Math::Vec2D& scl = Math::Vec2D(1.0f, 1.0f))
      : position(pos), rotation(rot), scale(scl) {
    }
    Transform2D(float x, float y, float rot, float scaleX, float scaleY)
      : position(x, y), rotation(rot), scale(scaleX, scaleY) {
    }
  };

}

// Lua-accessible (used for position checks in AI)
REGISTER_COMPONENT(Components::Transform2D, position, rotation, scale)
