//-----------------------------------------------------------------------------------
// file:    CatMonkeySummonWaveHealthBarsSystem.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//
// brief:   Draws 5 bottom-screen health bars (enemy healthbar sprite), one per
//          Cat Monkey summon wave.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

#include "../Components/CatMonkeySummonWaveHealthBarsComponent.h"
#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Resources.h"

namespace Engine::Systems
{
  class CatMonkeySummonWaveHealthBarsSystem final : public Engine::ECS::System
  {
  public:
    CatMonkeySummonWaveHealthBarsSystem()
    {
      require_components<Components::CatMonkeySummonWaveHealthBars>();
    }

    void draw_entity(const Engine::ECS::Entity e) override
    {
      auto* bars = e.get_component<Components::CatMonkeySummonWaveHealthBars>();
      if (!bars || !bars->visible) {
        return;
      }

      Engine::Graphics::Texture tex = Engine::Resources::load_texture("images/Enemyhealth_Spritesheet.png", true, true);
      if (!tex || tex->id == 0) {
        return;
      }

      const Math::Vec4D color_white{ 1.0f, 1.0f, 1.0f, 1.0f };

      const float spacing = std::max(0.0f, bars->spacing_px);
      const Math::Vec2D size = bars->bar_size_ui;
      const Math::Vec2D center = bars->center_ui;

      // 5 bars: offsets -2, -1, 0, 1, 2.
      draw_one(tex, color_white, center + Math::Vec2D(-2.0f * spacing, 0.0f), size,
        bars->wave1_current_hp, bars->wave1_max_hp, bars->wave1_spawned, bars->wave1_cleared, bars->wave1_cleared_time);
      draw_one(tex, color_white, center + Math::Vec2D(-1.0f * spacing, 0.0f), size,
        bars->wave2_current_hp, bars->wave2_max_hp, bars->wave2_spawned, bars->wave2_cleared, bars->wave2_cleared_time);
      draw_one(tex, color_white, center + Math::Vec2D(0.0f * spacing, 0.0f), size,
        bars->wave3_current_hp, bars->wave3_max_hp, bars->wave3_spawned, bars->wave3_cleared, bars->wave3_cleared_time);
      draw_one(tex, color_white, center + Math::Vec2D(1.0f * spacing, 0.0f), size,
        bars->wave4_current_hp, bars->wave4_max_hp, bars->wave4_spawned, bars->wave4_cleared, bars->wave4_cleared_time);
      draw_one(tex, color_white, center + Math::Vec2D(2.0f * spacing, 0.0f), size,
        bars->wave5_current_hp, bars->wave5_max_hp, bars->wave5_spawned, bars->wave5_cleared, bars->wave5_cleared_time);
    }

  private:
    static constexpr int kAtlasFrameCount = 11;

    static std::array<Math::Vec2D, 4> tex_coords_for_index(int atlas_index)
    {
      const int idx = std::clamp(atlas_index, 0, kAtlasFrameCount - 1);
      const float w = 1.0f / static_cast<float>(kAtlasFrameCount);
      const float x0 = w * static_cast<float>(idx);
      const float x1 = x0 + w;
      return { Math::Vec2D{ x0, 1.0f }, Math::Vec2D{ x1, 1.0f }, Math::Vec2D{ x1, 0.0f }, Math::Vec2D{ x0, 0.0f } };
    }

    static int hp_frame_index(int current_hp, int max_hp)
    {
      const int mx = std::max(0, max_hp);
      const int cur = std::max(0, current_hp);
      if (mx <= 0) {
        return 0;
      }

      const float ratio = std::clamp(static_cast<float>(std::min(cur, mx)) / static_cast<float>(mx), 0.0f, 1.0f);
      if (ratio > (4.0f / 5.0f)) return 0; // "1"
      if (ratio > (3.0f / 5.0f)) return 1; // "2"
      if (ratio > (2.0f / 5.0f)) return 2; // "3"
      if (ratio > (1.0f / 5.0f)) return 3; // "4"
      return 4; // "5"
    }

    static int death_frame_index(float time_since_clear)
    {
      // Matches Enemy_HealthBar.json Death: frames 7,8,9,10 each 0.15s.
      const float t = std::max(0.0f, time_since_clear);
      const int step = std::clamp(static_cast<int>(t / 0.15f), 0, 3);
      return 7 + step;
    }

    static bool death_anim_done(float time_since_clear)
    {
      return time_since_clear >= 0.60f;
    }

    static void draw_one(Engine::Graphics::Texture tex,
      const Math::Vec4D& color,
      const Math::Vec2D& pos,
      const Math::Vec2D& size,
      int current_hp,
      int max_hp,
      bool spawned,
      bool cleared,
      float cleared_time)
    {
      if (!spawned) {
        return;
      }

      int atlas_index = 0;
      if (cleared) {
        const float dt = Engine::Time::get_time() - cleared_time;
        if (death_anim_done(dt)) {
          return;
        }
        atlas_index = death_frame_index(dt);
      }
      else {
        atlas_index = hp_frame_index(current_hp, max_hp);
      }

      Engine::Graphics::draw_subtexture_ui(tex, tex_coords_for_index(atlas_index), pos, size, 0.0f, color);
    }
  };
}
