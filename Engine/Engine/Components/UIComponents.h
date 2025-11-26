//-----------------------------------------------------------------------------------
// file:    UIComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for user interface elements. Includes
//          Clickable component for handling mouse interactions and button
//          functionality. Provides the foundation for interactive UI elements
//          in the game and editor interface.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <functional>
#include "../ECS/Macros.h"
#include "CollisionComponents.h"

namespace Components
{
  struct Clickable
  {
    std::function<void()> on_click;

    std::function<void(Engine::ECS::Entity)> on_hover;

    std::function<void(Engine::ECS::Entity)> exit_hover;
  };

  // Dynamic button component that stores collider states for hover effects
  struct DynamicButton
  {
    // Normal state collider data
    f32 normal_width = 0.0f;
    f32 normal_height = 0.0f;
    std::vector<Math::Vec2D> normal_vertices;  // For polygon colliders

    // Hovered state collider data
    f32 hovered_width = 0.0f;
    f32 hovered_height = 0.0f;
    std::vector<Math::Vec2D> hovered_vertices;  // For polygon colliders

    // Track current hover state
    bool is_hovered = false;

    DynamicButton() = default;

    // Constructor for AABB colliders
    DynamicButton(f32 normal_w, f32 normal_h, f32 hovered_w, f32 hovered_h)
      : normal_width(normal_w)
      , normal_height(normal_h)
      , hovered_width(hovered_w)
      , hovered_height(hovered_h)
      , is_hovered(false)
    {
    }

    // Constructor for polygon colliders
    DynamicButton(const std::vector<Math::Vec2D>& normal_verts, const std::vector<Math::Vec2D>& hovered_verts)
      : normal_width(0.0f)
      , normal_height(0.0f)
      , normal_vertices(normal_verts)
      , hovered_width(0.0f)
      , hovered_height(0.0f)
      , hovered_vertices(hovered_verts)
      , is_hovered(false)
    {
    }
  };
}

REGISTER_COMPONENT(Components::DynamicButton, normal_width, normal_height, normal_vertices, hovered_width, hovered_height, hovered_vertices, is_hovered)