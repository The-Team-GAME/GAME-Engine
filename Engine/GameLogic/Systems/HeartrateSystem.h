
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
        require_components<Components::Heartrate>();

        font = Engine::Resources::load_font("fonts/Roboto-Regular.ttf", 48);
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* heartrate = entity.get_component<Components::Heartrate>();
        heartrate->decrease(heartrate->decay_rate * Engine::Time::get_fixed_dt());
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