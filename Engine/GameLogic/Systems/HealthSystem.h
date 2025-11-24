//-----------------------------------------------------------------------------------
// file:    HealthSystem.h
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//          Primary author
//
// email:   o.ziming@digipen.edu
//
// brief:   This file defines a system that handles the game logic of the health system
//          for the player and other entities in the game.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once


#include "../../Engine/MonkeyEngine.h"

#include "../Components/HealthComponents.h"

namespace Engine {
  namespace Systems
  {
    // Drives regen/drain and HUD for any entity with Components::Health
    class HealthSystem final : public Engine::ECS::System {
    public:
      HealthSystem() {
        require_components<Components::Health>();
      }

      void update_entity(Engine::ECS::Entity& e) override {
        using namespace Components;

        auto* hp = e.get_component<Health>();
        if (!hp) return;

        const float dt = Engine::Time::get_fixed_dt();

        // Regen/Drain
        if (auto* cfg = e.get_component<HealthConfig>()) {
          bool moving = false;
          if (auto* rb = e.get_component<RigidBody>()) {
            moving = (rb->velocity.get_magnitude_sq() > 0.01f);
          }
          float net = -cfg->drain_per_sec * dt;
          if (moving) net += cfg->heal_while_moving_per_sec * dt;
          cfg->carry += net;

          while (cfg->carry >= 1.0f) { hp->heal(1);   cfg->carry -= 1.0f; }
          while (cfg->carry <= -1.0f) { hp->damage(1); cfg->carry += 1.0f; }
        }

        // HUD
        if (auto* ui = e.get_component<HealthBar>()) {
          const float ratio = (hp->max > 0) ? (static_cast<float>(hp->value) / hp->max) : 0.0f;
          const float t = std::clamp(ratio, 0.0f, 1.0f);
          const float w = ui->width * t;

          if (ui->bar_fill.is_valid()) {
            auto bar = ui->bar_fill;
            if (auto* tr = bar.get_component<Transform2D>()) {
              tr->scale.x = w;
              tr->scale.y = ui->height;
              tr->position.x = ui->left + w * 0.5f;
              tr->position.y = ui->top + ui->height * 0.5f;
            }
            if (auto* c = bar.get_component<Color>()) {
              const float t2 = t * t;
              float r = t2, g = 0.1f, b = 1.0f - t2, a = 1.0f;
              if (t >= 0.80f) {
                const float time = Engine::Time::get_time();
                const float s = 0.5f * (std::sin(time * 6.0f) + 1.0f);
                const float baseA = 0.65f, ampA = 0.35f;
                a = baseA + ampA * s;
                const float glow = 0.90f + 0.10f * s;
                r *= glow; g *= glow; b *= glow;
              }
              c->r = r * 0.9f + 0.1f;
              c->g = g;
              c->b = b * 0.9f + 0.1f;
              c->a = a;
            }
          }

          if (ui->text.is_valid()) {
            auto txt = ui->text;
            if (auto* tcomp = txt.get_component<Text>()) {
              tcomp->content = "Player HP: " + std::to_string(hp->value) + "/" + std::to_string(hp->max);
            }
          }
        }
        bool is_player = e.has_component<Tag<"Player">>();
        if (hp->value <= 0 || (is_player && hp->value >= hp->max)) {
          e.destroy();
        }
      }
    };

    class EnemyHealthBarSystem : public Engine::ECS::System {
    public:
      EnemyHealthBarSystem() {
        require_components<Components::EnemyHealth, Components::Transform2D, Components::EnemyHealthBar>();
      }

      void update_entity(Engine::ECS::Entity& e) override {
        using namespace Components;
        auto* hp = e.get_component<EnemyHealth>();
        auto* tr = e.get_component<Components::Transform2D>();
        auto* bar = e.get_component<EnemyHealthBar>();
        if (!hp || !tr || !bar) return;

        ensure_bar(e, *bar);

        const float now = Engine::Time::get_time();
        const float dt = Engine::Time::get_fixed_dt();
        const int   cur = std::max(0, hp->current_hp);
        const int   mx = std::max(1, hp->max_hp);

        if (bar->last_hp < 0) { // first tick
          bar->last_hp = cur; bar->last_max = mx;
          bar->front_ratio = bar->after_ratio = static_cast<float>(cur) / mx;
        }

        // Damage
        if (cur < bar->last_hp) {
          const float newR = static_cast<float>(cur) / mx;
          const float oldR = static_cast<float>(bar->last_hp) / mx;
          bar->front_ratio = newR;            // snap
          bar->after_ratio = oldR;            // start draining from previous
          bar->visible_until = now + bar->show_duration;
        }
        // Heal: both jump up
        else if (cur > bar->last_hp) {
          const float r = static_cast<float>(cur) / mx;
          bar->front_ratio = bar->after_ratio = r;
          bar->visible_until = now + 0.4f;
        }

        bar->last_hp = cur; bar->last_max = mx;

        // Drain delayed
        if (bar->after_ratio > bar->front_ratio) {
          bar->after_ratio = std::max(bar->front_ratio, bar->after_ratio - bar->drain_speed * dt);
        }

        const bool show = (now <= bar->visible_until) && !hp->is_dead();
        if (show) {
          layout_arc(e, *bar);
        }
        else {
          set_layer_visibility(bar->front, 0);
          set_layer_visibility(bar->after, 0);
        }

        if (hp->is_dead()) {
          for (auto& seg : bar->front) if (seg.is_valid()) seg.destroy();
          for (auto& seg : bar->after) if (seg.is_valid()) seg.destroy();
          bar->front.clear(); bar->after.clear();
          bar->initialized = false;
        }
      }

    private:
      static inline float deg2rad(float d) { return d * 0.01745329251994329577f; }

      static Engine::Graphics::Texture default_rect() {
        return Engine::Resources::get_rect_texture();
      }

      void ensure_bar(Engine::ECS::Entity& owner, Components::EnemyHealthBar& bar) {
        using namespace Components;
        if (bar.initialized) return;
        bar.initialized = true;
        if (!bar.tex) bar.tex = default_rect();

        const int n = std::max(3, bar.segments);
        bar.front.resize(n);
        bar.after.resize(n);

        // Pre-create segment sprites
        for (int i = 0; i < n; ++i) {
          {
            auto seg = Engine::ECS::create_entity()
              .add_component<Components::Transform2D>(0.f, 0.f, 0.f, bar.after_thickness, bar.after_thickness)
              .add_component<Components::Texture>(bar.tex)
              .add_component<Components::Color>(bar.after_color.r, bar.after_color.g, bar.after_color.b, bar.after_color.a);
            bar.after[i] = seg;
          }
          {
            auto seg = Engine::ECS::create_entity()
              .add_component<Components::Transform2D>(0.f, 0.f, 0.f, bar.front_thickness, bar.front_thickness)
              .add_component<Components::Texture>(bar.tex)
              .add_component<Components::Color>(bar.front_color.r, bar.front_color.g, bar.front_color.b, bar.front_color.a);
            bar.front[i] = seg;
          }
        }

        tint_layer(bar.front, bar.front_color);
        tint_layer(bar.after, bar.after_color);

        set_layer_visibility(bar.front, 0);
        set_layer_visibility(bar.after, 0);
      }

      void set_layer_visibility(std::vector<Engine::ECS::Entity>& layer, int visible_count) {
        const int n = static_cast<int>(layer.size());
        for (int i = 0; i < n; ++i) {
          if (!layer[i].is_valid()) continue;
          bool vis = (i < visible_count);
          if (auto* tr = layer[i].get_component<Components::Transform2D>()) {
            tr->scale.y = vis ? tr->scale.x : 0.0f; // hide by collapsing Y
          }
          if (auto* c = layer[i].get_component<Components::Color>()) {
            c->a = vis ? c->a : 0.0f;
          }
        }
      }

      void tint_layer(std::vector<Engine::ECS::Entity>& layer, const Components::EnemyHealthBar::RGBA& c) {
        for (auto& seg : layer) {
          if (!seg.is_valid()) continue;
          if (auto* col = seg.get_component<Components::Color>()) {
            col->r = c.r; col->g = c.g; col->b = c.b; col->a = c.a;
          }
        }
      }

      void layout_arc(Engine::ECS::Entity& owner, Components::EnemyHealthBar& bar) {
        auto* tOwner = owner.get_component<Components::Transform2D>();
        if (!tOwner) return;

        const int n = std::max(3, bar.segments);
        const float a0 = deg2rad(bar.start_angle_deg);
        const float a1 = deg2rad(bar.end_angle_deg);
        const float span = (a1 - a0);

        auto place_ring = [&](std::vector<Engine::ECS::Entity>& ring, float thickness, float ratio) {
          const int visible = std::clamp(static_cast<int>(std::round(ratio * n)), 0, n);
          for (int i = 0; i < n; ++i) {
            if (!ring[i].is_valid()) continue;
            const float t = (i + 0.5f) / n;
            const float ang = a0 + t * span;
            const float cx = tOwner->position.x + bar.offset.x + std::cos(ang) * bar.radius;
            const float cy = tOwner->position.y + bar.offset.y + std::sin(ang) * bar.radius;

            if (auto* tr = ring[i].get_component<Components::Transform2D>()) {
              tr->position.x = cx;
              tr->position.y = cy;
              tr->rotation = -ang;     // face outward
              tr->scale.x = thickness;
              tr->scale.y = thickness; // toggled by visibility
            }
          }
          set_layer_visibility(ring, visible);
          };

        place_ring(bar.after, bar.after_thickness, bar.after_ratio);
        place_ring(bar.front, bar.front_thickness, bar.front_ratio);
      }
    };

  }
}