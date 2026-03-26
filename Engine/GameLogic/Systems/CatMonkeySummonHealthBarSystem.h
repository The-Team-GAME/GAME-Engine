//-----------------------------------------------------------------------------------
// file:    CatMonkeySummonHealthBarSystem.h
// project: Monkey Stole My GPA
//
// brief:   Draws a screen-space rectangle health bar representing the combined
//          HP pool of Cat Monkey's currently-summoned enemies.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

#include "../Components/CatMonkeySummonHealthBarComponent.h"
#include "../../Engine/Graphics/Renderer.h"

namespace Engine::Systems
{
  class CatMonkeySummonHealthBarSystem final : public Engine::ECS::System
  {
  public:
    CatMonkeySummonHealthBarSystem()
    {
      require_components<Components::CatMonkeySummonHealthBar>();
    }

    void draw_entity(const Engine::ECS::Entity e) override
    {
      auto* bar = e.get_component<Components::CatMonkeySummonHealthBar>();
      if (!bar || !bar->visible) {
        return;
      }

      const int mx = std::max(0, bar->total_max_hp);
      const int cur = std::max(0, bar->total_current_hp);
      const float ratio = (mx > 0)
        ? std::clamp(static_cast<float>(std::min(cur, mx)) / static_cast<float>(mx), 0.0f, 1.0f)
        : 0.0f;

      const Math::Vec2D center = bar->center_ui;
      const Math::Vec2D size = bar->size_ui;
      const float border = std::max(0.0f, bar->border_thickness_px);

      // Simple, readable palette.
      const Math::Vec4D border_col{ 0.0f, 0.0f, 0.0f, 0.85f };
      const Math::Vec4D back_col{ 0.0f, 0.0f, 0.0f, 0.35f };
      const Math::Vec4D fill_col{ 0.85f, 0.10f, 0.10f, 0.90f };

      // Border / outline
      if (border > 0.0f) {
        Engine::Graphics::draw_rect_outline_ui(center, size, 0.0f, border, border_col);
      }

      // Background
      Engine::Graphics::draw_rect_ui(center, size, 0.0f, back_col);

      // Fill (anchored to the left)
      const float fill_w = size.x * ratio;
      if (fill_w > 0.0f) {
        const float left_x = center.x - size.x * 0.5f;
        Math::Vec2D fill_center{ left_x + fill_w * 0.5f, center.y };
        Math::Vec2D fill_size{ fill_w, size.y };
        Engine::Graphics::draw_rect_ui(fill_center, fill_size, 0.0f, fill_col);
      }
    }
  };
}
