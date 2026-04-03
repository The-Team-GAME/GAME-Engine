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

        auto* player = player_entity.get_component<Components::Player>();
        if (!player) {
          return;
        }

        static auto font = Engine::Resources::load_font("fonts/Roboto-Regular.ttf", 20);
        if (!font) {
          return;
        }

        const std::array<std::pair<std::string, bool>, 4> cheat_states = {{
          { "F5 Dash CD: " + status_text(player->dash_cooldown_cheat), player->dash_cooldown_cheat },
          { "F6 Uppercut CD: " + status_text(player->uppercut_cooldown_cheat), player->uppercut_cooldown_cheat },
          { "F7 Smash CD: " + status_text(player->smash_cooldown_cheat), player->smash_cooldown_cheat },
          { "F8 God Mode: " + status_text(player->god_mode), player->god_mode }
        }};

        constexpr f32 scale = 0.7f;
        constexpr f32 line_spacing = 6.f;
        constexpr f32 padding = 16.f;
        const Math::Vec2D virtual_resolution = Engine::Graphics::get_virtual_resolution();

        f32 total_height = 0.f;
        for (const auto& [text, enabled] : cheat_states) {
          (void)enabled;
          total_height += Engine::Graphics::measure_text(font, text, scale).y;
        }
        total_height += line_spacing * static_cast<f32>(cheat_states.size() - 1);

        f32 y = virtual_resolution.y - total_height - padding;
        for (const auto& [text, enabled] : cheat_states) {
          const Math::Vec2D text_size = Engine::Graphics::measure_text(font, text, scale);
          const f32 x = virtual_resolution.x - text_size.x - padding;
          const Math::Vec4D color = enabled
            ? Math::Vec4D(0.45f, 1.0f, 0.55f, 1.0f)
            : Math::Vec4D(0.90f, 0.90f, 0.90f, 0.9f);
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