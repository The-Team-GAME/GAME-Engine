//-----------------------------------------------------------------------------------
// file:    UIUtils.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Shared UI utility helpers for creating common ECS-based UI elements
//          (screen dimmers, overlays, etc.) to avoid duplicating boilerplate.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "ECS/ECS.h"
#include "Components/Components.h"
#include "Resources.h"
#include "Graphics/Renderer.h"

namespace Engine::UI
{
  /// Creates a full-screen black translucent overlay entity and returns its ID.
  /// @param alpha       Opacity of the dimmer (0 = invisible, 1 = fully black).
  /// @param layer       Render layer name to place the entity on.
  /// @param runtime_only Whether to tag the entity as RuntimeOnly (not serialised).
  inline Engine::ECS::EntityID create_screen_dimmer(
    float alpha = 0.5f,
    const std::string& layer = "overlay_bg",
    bool runtime_only = false)
  {
    Math::Vec2D vres = Engine::Graphics::get_virtual_resolution();
    float width  = vres.x;
    float height = vres.y;

    auto e = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(width * 0.5f, height * 0.5f, 0.0f, width, height)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::Color>(0.0f, 0.0f, 0.0f, alpha)
      .add_component<Components::UI>();

    if (runtime_only)
      e.add_component<Components::RuntimeOnly>();

    e.add_to_layer(layer);
    return e.get_id();
  }
} // namespace Engine::UI


