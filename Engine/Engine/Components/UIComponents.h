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
    struct StateData
    {
      f32 width = 0.0f;
      f32 height = 0.0f;
      std::vector<Math::Vec2D> vertices;  // For polygon colliders

      // Transform data
      Components::Transform2D transform;
    };

    StateData normal;
    StateData hovered;
    StateData clicked;

    // Track current hover and click state
    bool is_hovered = false;
    bool is_clicked = false;

    DynamicButton() = default;

    // Constructor for AABB colliders
    DynamicButton(f32 normal_w, f32 normal_h, f32 hovered_w, f32 hovered_h, f32 clicked_w, f32 clicked_h)
      : is_hovered(false)
      , is_clicked(false)
    {
      normal.width = normal_w;
      normal.height = normal_h;
      hovered.width = hovered_w;
      hovered.height = hovered_h;
      clicked.width = clicked_w;
      clicked.height = clicked_h;
    }

    // Constructor for polygon colliders
    DynamicButton(const std::vector<Math::Vec2D>& normal_verts, const std::vector<Math::Vec2D>& hovered_verts, const std::vector<Math::Vec2D>& clicked_verts)
      : is_hovered(false)
      , is_clicked(false)
    {
      normal.vertices = normal_verts;
      hovered.vertices = hovered_verts;
      clicked.vertices = clicked_verts;
    }
  };
}

REFLECT_FIELDS(Components::DynamicButton::StateData, width, height, vertices, transform)
REGISTER_COMPONENT(Components::DynamicButton, normal, hovered, clicked, is_hovered, is_clicked)