//-----------------------------------------------------------------------------------
// file:    CheatOverlaySystem.h
// project: Monkey Stole My GPA
//
// brief:   Draws the runtime cheat status overlay in the bottom-right corner.
//
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"
#include "../Components/GameplayComponent.h"

#include <array>
#include <cstddef>
#include <string>

namespace Engine {
  namespace Systems {

    class CheatOverlaySystem : public Engine::ECS::System {
    public:
      void draw() override {
        auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
        if (!player_entity.is_valid()) {
          return;
        }

        auto* player_comp = player_entity.get_component<Components::Player>();
        if (!player_comp) {
          return;
        }

        static auto font = Engine::Resources::load_font("fonts/Roboto-Regular.ttf", 20);
        if (!font) {
          return;
        }

        const std::array<std::pair<std::string, bool>, 2> cheat_states = {{
          { "F7 No Cooldown: " + status_text(player_comp->no_cooldown_cheat), player_comp->no_cooldown_cheat },
          { "F8 God Mode: " + status_text(player_comp->god_mode), player_comp->god_mode }
        }};

        constexpr f32 scale = 0.7f;
        constexpr f32 line_spacing = 6.f;
        constexpr f32 padding = 16.f;
        const Math::Vec2D virtual_resolution = Engine::Graphics::get_virtual_resolution();

        f32 total_height = 0.f;
        std::size_t enabled_cheat_count = 0;
        for (const auto& [text, enabled] : cheat_states) {
          if (!enabled) {
            continue;
          }

          ++enabled_cheat_count;
          total_height += Engine::Graphics::measure_text(font, text, scale).y;
        }

        if (enabled_cheat_count == 0) {
          return;
        }

        if (enabled_cheat_count > 1) {
          total_height += line_spacing * static_cast<f32>(enabled_cheat_count - 1);
        }

        f32 y = virtual_resolution.y - total_height - padding;
        for (const auto& [text, enabled] : cheat_states) {
          if (!enabled) {
            continue;
          }

          const Math::Vec2D text_size = Engine::Graphics::measure_text(font, text, scale);
          const f32 x = virtual_resolution.x - text_size.x - padding;
          const Math::Vec4D color = Math::Vec4D(0.45f, 1.0f, 0.55f, 1.0f);
          Engine::Graphics::draw_text_ui(font, text, { x, y }, scale, color);
          y += text_size.y + line_spacing;
        }
      }

    private:
      static std::string status_text(bool enabled) {
        return enabled ? "Enabled" : "Disabled";
      }
    };

  } // namespace Systems
} // namespace Engine