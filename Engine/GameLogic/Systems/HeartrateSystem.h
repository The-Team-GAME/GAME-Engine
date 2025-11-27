
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
        require_components<Components::Heartrate, Components::Player, Components::Color, Components::Trail, Components::Distortion>();

        font = Engine::Resources::load_font("fonts/Roboto-Regular.ttf", 48);
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* heartrate = entity.get_component<Components::Heartrate>();
        auto* player = entity.get_component<Components::Player>();
        auto* color = entity.get_component<Components::Color>();
        auto* trail = entity.get_component<Components::Trail>();
        auto* distortion = entity.get_component<Components::Distortion>();


        heartrate->decrease(heartrate->decay_rate * Engine::Time::get_fixed_dt());

        // Multiply color (tint) all tilemap tiles based on heartrate state, the higher the heartrate, the greener the tint
        Components::Heartrate::HeartrateState state = heartrate->get_state();

        Math::Vec4D tint_color = { 1.0f, 1.0f, 1.0f, 1.0f };

        switch (state) {
        case Components::Heartrate::HeartrateState::Low:
          tint_color = heartrate->low_world_tint;
          break;
        case Components::Heartrate::HeartrateState::Normal:
          tint_color = heartrate->normal_world_tint;
          break;
        case Components::Heartrate::HeartrateState::High:
          tint_color = heartrate->high_world_tint;
          break;
        case Components::Heartrate::HeartrateState::VeryHigh:
          tint_color = heartrate->very_high_world_tint;
          break;
        case Components::Heartrate::HeartrateState::CrashOut:
          tint_color = heartrate->crash_out_world_tint;
          break;
        case Components::Heartrate::HeartrateState::Dead:
          tint_color = heartrate->dead_world_tint;
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
          if (tile_color) {
            float lerp_speed = 1.0f;
            float dt = Engine::Time::get_fixed_dt();

            tile_color->r += (tint_color.x - tile_color->r) * lerp_speed * dt;
            tile_color->g += (tint_color.y - tile_color->g) * lerp_speed * dt;
            tile_color->b += (tint_color.z - tile_color->b) * lerp_speed * dt;
            tile_color->a += (tint_color.w - tile_color->a) * lerp_speed * dt;
          }
        }

        // Apply rainbow tint to player if in CrashOut state 
        if (state == Components::Heartrate::HeartrateState::CrashOut || state == Components::Heartrate::HeartrateState::Dead) {
          float time = Engine::Time::get_time();
          float min = heartrate->min_brightness;
          float max = heartrate->max_brightness;
          float amplitude = (max - min) * 0.5f;
          float center = min + amplitude;

          float r = std::sin(time * 5.0f) * amplitude + center;
          float g = std::sin(time * 5.0f + 2.0f) * amplitude + center;
          float b = std::sin(time * 5.0f + 4.0f) * amplitude + center;
          color->set_rgba(r, g, b, 1.0f);
        }
        else {
          color->set_rgba(1.0f, 1.0f, 1.0f, 1.0f);
        }

        // Adjust trail and distortion lifetime based on heartrate
        if (trail) {
          float lifetime = 0.0f;
          float distortion_intensity = 0.0f;
          float distortion_speed = 0.0f;
          float mesh_distortion_intensity = 0.0f;
          switch (state) {
          case Components::Heartrate::HeartrateState::Low:
            lifetime = heartrate->low_trail_lifetime;
            distortion_intensity = heartrate->low_distortion_intensity;
            distortion_speed = heartrate->low_distortion_speed;
            mesh_distortion_intensity = heartrate->low_mesh_distortion_intensity;
            break;
          case Components::Heartrate::HeartrateState::Normal:
            lifetime = heartrate->normal_trail_lifetime;
            distortion_intensity = heartrate->normal_distortion_intensity;
            distortion_speed = heartrate->normal_distortion_speed;
            mesh_distortion_intensity = heartrate->normal_mesh_distortion_intensity;
            break;
          case Components::Heartrate::HeartrateState::High:
            lifetime = heartrate->high_trail_lifetime;
            distortion_intensity = heartrate->high_distortion_intensity;
            distortion_speed = heartrate->high_distortion_speed;
            mesh_distortion_intensity = heartrate->high_mesh_distortion_intensity;
            break;
          case Components::Heartrate::HeartrateState::VeryHigh:
            lifetime = heartrate->very_high_trail_lifetime;
            distortion_intensity = heartrate->very_high_distortion_intensity;
            distortion_speed = heartrate->very_high_distortion_speed;
            mesh_distortion_intensity = heartrate->very_high_mesh_distortion_intensity;
            break;
          case Components::Heartrate::HeartrateState::CrashOut:
            lifetime = heartrate->crash_out_trail_lifetime;
            distortion_intensity = heartrate->crash_out_distortion_intensity;
            distortion_speed = heartrate->crash_out_distortion_speed;
            mesh_distortion_intensity = heartrate->crash_out_mesh_distortion_intensity;
            break;
          case Components::Heartrate::HeartrateState::Dead:
            lifetime = heartrate->dead_trail_lifetime;
            distortion_intensity = heartrate->dead_distortion_intensity;
            distortion_speed = heartrate->dead_distortion_speed;
            mesh_distortion_intensity = heartrate->dead_mesh_distortion_intensity;
            break;
          }
          trail->lifetime = lifetime;
          distortion->intensity = distortion_intensity;
          distortion->speed = distortion_speed;
          distortion->mesh_intensity = mesh_distortion_intensity;
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