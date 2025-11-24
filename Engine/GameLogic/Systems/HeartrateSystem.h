
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../Components/HeartrateComponent.h"

namespace Engine {
  namespace Systems
  {
    class HeartrateSystem : public Engine::ECS::System {
    private:
      Engine::Graphics::Font font;
    public:
      HeartrateSystem() {
        require_components<Components::Heartrate, Components::Player, Components::Color>();

        font = Engine::Resources::load_font("fonts/Roboto-Regular.ttf", 48);
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* heartrate = entity.get_component<Components::Heartrate>();
        auto* player = entity.get_component<Components::Player>();
        auto* color = entity.get_component<Components::Color>();

        heartrate->decrease(heartrate->decay_rate * Engine::Time::get_fixed_dt());

        // Multiply color (tint) all tilemap tiles based on heartrate state, the higher the heartrate, the greener the tint
        Components::Heartrate::HeartrateState state = heartrate->get_state();

        Math::Vec4D tint_color = { 1.0f, 1.0f, 1.0f, 1.0f };

        switch (state) {
        case Components::Heartrate::HeartrateState::Low:
          tint_color = heartrate->low_tint;
          break;
        case Components::Heartrate::HeartrateState::Normal:
          tint_color = heartrate->normal_tint;
          break;
        case Components::Heartrate::HeartrateState::High:
          tint_color = heartrate->high_tint;
          break;
        case Components::Heartrate::HeartrateState::VeryHigh:
          tint_color = heartrate->very_high_tint;
          break;
        case Components::Heartrate::HeartrateState::CrashOut:
          tint_color = heartrate->crash_out_tint;
          break;
        case Components::Heartrate::HeartrateState::Dead:
          tint_color = heartrate->dead_tint;
          break;
        }

        // tint_color.x = ((heartrate->crash_out - heartrate->low) - heartrate->heartrate - heartrate->low) / (heartrate->crash_out - heartrate->low);
        // tint_color.z = ((heartrate->crash_out - heartrate->low) - heartrate->heartrate - heartrate->low) / (heartrate->crash_out - heartrate->low);

        // Apply tint to all tilemap entities
        auto tile_entities = Engine::ECS::get_entities_with<Components::TileMap>();
        for (auto& tilemap_entity : tile_entities) {
          // std::cout << "Applying tint to tilemap entity ID: " << tilemap_entity.get_id() << std::endl;
          if (!tilemap_entity.has_component<Components::Color>()) {
            tilemap_entity.add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);
          }
          auto* tile_color = tilemap_entity.get_component<Components::Color>();
          if (tile_color) *tile_color = tint_color;
        }

        // Apply rainbow tint to player if in CrashOut state
        auto* player_color = entity.get_component<Components::Color>();

        if (state == Components::Heartrate::HeartrateState::CrashOut || state == Components::Heartrate::HeartrateState::Dead) {
          float time = Engine::Time::get_time();
          float r = std::sin(time * 5.0f) * 0.5f + 0.5f;
          float g = std::sin(time * 5.0f + 2.0f) * 0.5f + 0.5f;
          float b = std::sin(time * 5.0f + 4.0f) * 0.5f + 0.5f;
          player_color->set_rgba(r, g, b, 1.0f);
        }
        else {
          player_color->set_rgba(1.0f, 1.0f, 1.0f, 1.0f);
        }
      }


      void draw_entity(const Engine::ECS::Entity& entity) override {
        auto* heartrate = entity.get_component<Components::Heartrate>();
        if (!heartrate) return;


        // Background rectangle
        Engine::Graphics::draw_rect({ 10.0f, 10.0f }, { 200.0f, 10.0f }, 0.0f, { 1.0f, 1.0f, 1.0f, 1.0f });

        // Heartrate bar
        /*f32 hr_ratio = heartrate->heartrate / heartrate->crash_out;
        Engine::Graphics::draw_rect({ 10.0f, 10.0f }, { 200.0f * hr_ratio, 10.0f }, 0.0f, { 1.0f, 0.0f, 0.0f, 1.0f });*/

        // Draw text
        Engine::Graphics::draw_text(
          font,
          "Heartrate: " + std::to_string(static_cast<int>(heartrate->heartrate)) + " bpm",
          { 10.0f, 25.0f },
          1.0f,
          { 1.0f, 1.0f, 1.0f, 1.0f }
        );

      }
    };
  }
}