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
#include "../Components/HeartrateComponent.h"

#include "../Components/EnemyHealthComponent.h"

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
          bar->has_been_hit = true;
          if (!bar->persist_after_first_hit) 
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

        const bool show = bar->persist_after_first_hit
          ? (bar->has_been_hit && !hp->is_dead())
          : (now <= bar->visible_until && !hp->is_dead());

        if (show) layout_arc(e, *bar);
        else { set_layer_visibility(bar->front, 0); set_layer_visibility(bar->after, 0); }

        if (hp->is_dead()) {
          for (auto& seg : bar->front) if (seg.is_valid()) seg.destroy();
          for (auto& seg : bar->after) if (seg.is_valid()) seg.destroy();
          bar->front.clear(); bar->after.clear();
          bar->initialized = false;
        }
      }

    private:
      static inline float deg2rad(float d) { return d * 0.01745329251994329577f; }
      static inline float rad2deg(float r) { return r * 57.29577951308232f; }

      static Engine::Graphics::Texture default_rect() { return Engine::Resources::get_rect_texture(); }

      // normalize to [0, 2π), and shortest signed delta in (-π, π]
      static inline float norm2pi(float a) { const float tau = 6.28318530718f; a = fmodf(a, tau); return (a < 0.f) ? a + tau : a; }
      static inline float short_delta(float a, float b) {
        float d = b - a; const float pi = 3.14159265359f, tau = 6.28318530718f;
        while (d > pi) d -= tau;
        while (d <= -pi) d += tau;
        return d;
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
              .add_component<Components::Color>(bar.after_color.r, bar.after_color.g, bar.after_color.b, bar.after_color.a)
              .remove_from_layer("default")
              .add_to_layer(bar.layer.c_str());

            bar.after[i] = seg;
          }
          {
            auto seg = Engine::ECS::create_entity()
              .add_component<Components::Transform2D>(0.f, 0.f, 0.f, bar.front_thickness, bar.front_thickness)
              .add_component<Components::Texture>(bar.tex)
              .add_component<Components::Color>(bar.front_color.r, bar.front_color.g, bar.front_color.b, bar.front_color.a)
              .remove_from_layer("default")
              .add_to_layer(bar.layer.c_str());
            bar.front[i] = seg;
          }
        }

        set_layer_visibility(bar.front, 1);
        set_layer_visibility(bar.after, 1);
      }

      void set_layer_visibility(std::vector<Engine::ECS::Entity>& layer, int visible_from_right) {
        const int n = static_cast<int>(layer.size());
        for (int i = 0; i < n; ++i) {
          if (!layer[i].is_valid()) continue;
          const bool vis = (i >= n - visible_from_right); // R→L fill
          if (auto* tr = layer[i].get_component<Components::Transform2D>()) {
            // keep previous width; toggle only height
            tr->scale.y = vis ? tr->scale.y : 0.0f;
          }
        }
      }

      // Smooth continuous arc using stretched quads; Y-down coordinates
      void layout_arc(Engine::ECS::Entity& owner, Components::EnemyHealthBar& bar) {
        auto* tOwner = owner.get_component<Components::Transform2D>();
        if (!tOwner) return;

        const int n = std::max(8, bar.segments);          // more segments = smoother
        const float DEG = 0.01745329251994329577f;

        float a0_deg, a1_deg;
        if (bar.use_center_span) {
          const float half = std::max(0.f, bar.arc_span_deg) * 0.5f;
          a0_deg = bar.center_angle_deg - half;
          a1_deg = bar.center_angle_deg + half;
        }
        else {
          a0_deg = bar.start_angle_deg;
          a1_deg = bar.end_angle_deg;
        }
        auto norm2pi = [](float a) { const float tau = 6.28318530718f; a = fmodf(a, tau); return (a < 0.f) ? a + tau : a; };
        auto short_delta = [&](float a, float b) {
          float d = b - a; const float pi = 3.14159265359f, tau = 6.28318530718f;
          while (d > pi) d -= tau; while (d <= -pi) d += tau; return d;
          };
        float a0 = norm2pi(a0_deg * DEG), a1 = norm2pi(a1_deg * DEG);
        const float span = short_delta(a0, a1);

        const int vis_after = std::clamp(int(std::round(bar.after_ratio * n)), 0, n);
        const int vis_front = std::clamp(int(std::round(bar.front_ratio * n)), 0, n);

        const float overlap = 1.02f;

        auto place_ring = [&](std::vector<Engine::ECS::Entity>& ring, float thickness, int visible_from_right) {
          for (int i = 0; i < n; ++i) {
            if (!ring[i].is_valid()) continue;

            // Segment angular range
            const float t0 = float(i) / n, t1 = float(i + 1) / n;
            const float ang0 = a0 + t0 * span;
            const float ang1 = a0 + t1 * span;

            // Endpoints (Y-down)
            const float x0 = tOwner->position.x + bar.offset.x + std::cos(ang0) * bar.radius;
            const float y0 = tOwner->position.y + bar.offset.y - std::sin(ang0) * bar.radius;
            const float x1 = tOwner->position.x + bar.offset.x + std::cos(ang1) * bar.radius;
            const float y1 = tOwner->position.y + bar.offset.y - std::sin(ang1) * bar.radius;

            // Center, angle, length
            const float cx = (x0 + x1) * 0.5f;
            const float cy = (y0 + y1) * 0.5f;
            const float ang = std::atan2(y1 - y0, x1 - x0);
            const float len = std::hypot(x1 - x0, y1 - y0) * overlap;

            // R→L visibility: show last 'visible_from_right' segments
            const bool visible = (i >= n - visible_from_right);

            if (auto* tr = ring[i].get_component<Components::Transform2D>()) {
              tr->position.x = cx;
              tr->position.y = cy;
              tr->rotation = ang;          // align along the arc
              tr->scale.x = len;          // LONG side along tangent
              tr->scale.y = visible ? thickness : 0.0f;  // thickness or hidden
            }
          }
          };

        place_ring(bar.after, bar.after_thickness, vis_after);
        place_ring(bar.front, bar.front_thickness, vis_front);
      }

    };

    class HeartHUDSpriteSystem final : public Engine::ECS::System {
    public:
      HeartHUDSpriteSystem() { require_components<Components::Heartrate, Components::HeartHUDSprite>(); }

      void update_entity(Engine::ECS::Entity& e) override {
        using namespace Components;
        auto* hr = e.get_component<Heartrate>();
        auto* hud = e.get_component<HeartHUDSprite>();
        if (!hr || !hud) return;

        ensure_created(*hud);

        // Effective anchors
        const float headX = hud->anchor_px.x + hud->head_offset_px.x;
        const float headY = hud->anchor_px.y + hud->head_offset_px.y;
        const float backX = hud->anchor_px.x + hud->back_offset_px.x;
        const float backY = hud->anchor_px.y + hud->back_offset_px.y;

        // Back
        if (auto* t = hud->back.get_component<Components::Transform2D>()) {
          t->position.x = backX + hud->width * 0.5f;
          t->position.y = backY + hud->height * 0.5f;
          t->rotation = 0.f; t->scale.x = hud->width; t->scale.y = hud->height;
        }
        // Overlay
        if (hud->use_white_overlay && hud->white_overlay.is_valid()) {
          if (auto* t = hud->white_overlay.get_component<Components::Transform2D>()) {
            const float baseX = hud->overlay_relative_to_back ? backX : hud->anchor_px.x;
            const float baseY = hud->overlay_relative_to_back ? backY : hud->anchor_px.y;
            const float ovX = baseX + hud->overlay_offset_px.x;
            const float ovY = baseY + hud->overlay_offset_px.y;
            t->position.x = ovX + hud->width * 0.5f;
            t->position.y = ovY + hud->height * 0.5f;
            t->rotation = 0.f; t->scale.x = hud->width; t->scale.y = hud->height;
          }
        }
        // Head
        if (auto* t = hud->head.get_component<Components::Transform2D>()) {
          t->position.x = headX + 60.f; t->position.y = headY + 58.f;
          t->rotation = 0.f; t->scale.x = 120.f; t->scale.y = 120.f;
        }

        // BPM visible columns
        const float bpm = std::max(1.f, hr->heartrate);
        const float span = std::max(1e-3f, hud->bpm_max - hud->bpm_min);
        const float ratio = std::clamp((bpm - hud->bpm_min) / span, 0.f, 1.f);

        // ---BPM-driven tint ---
        if (hud->use_bpm_tint) {
          Math::Vec4D c;
          c.x = hud->fill_color_low.x + (hud->fill_color_high.x - hud->fill_color_low.x) * ratio;
          c.y = hud->fill_color_low.y + (hud->fill_color_high.y - hud->fill_color_low.y) * ratio;
          c.z = hud->fill_color_low.z + (hud->fill_color_high.z - hud->fill_color_low.z) * ratio;
          c.w = hud->fill_color_low.w + (hud->fill_color_high.w - hud->fill_color_low.w) * ratio;
          for (auto& s : hud->whiteSlices) if (auto* col = s.get_component<Color>()) *col = c;
        }

        const int n = std::max(1, hud->slices);
        const int vis = std::clamp(int(std::round(ratio * n)), 0, n);

        place_white_columns(*hud, backX, backY, vis);
        promote_overlay_to_front(*hud);
        promote_head_to_front(*hud);
      }

    private:
      static Engine::Graphics::Texture rect() { return Engine::Resources::get_rect_texture(); }

      void ensure_created(Components::HeartHUDSprite& hud) {
        using namespace Components;
        if (hud.initialized) return;
        hud.initialized = true;

        if (!hud.tex_back)  hud.tex_back = rect();
        if (!hud.tex_solid) hud.tex_solid = rect();
        if (!hud.tex_head)  hud.tex_head = rect();

        // Back
        hud.back = Engine::ECS::create_entity()
          .add_component<Components::Transform2D>(0.f, 0.f, 0.f, hud.width, hud.height)
          .add_component<Components::Texture>(hud.tex_back)
          .add_component<Components::Color>(Math::Vec4D(1, 1, 1, 1))
          .remove_from_layer("default").add_to_layer(hud.layer.c_str());

        // --- use low color when creating each slice (was pure white) ---
        const int srcCols = std::max(1, hud.src_w_px);
        const int cropL = std::clamp(hud.src_left_px, 0, hud.src_w_px - 1);
        const int cropR = std::clamp(hud.src_right_px, cropL + 1, hud.src_w_px);
        const int n = std::max(1, cropR - cropL);
        hud.slices = n;

        hud.whiteSlices.resize(n);
        for (int i = 0; i < n; ++i) {
          auto col = Engine::ECS::create_entity()
            .add_component<Components::Transform2D>(0.f, 0.f, 0.f, 1.f, 1.f)
            .add_component<Components::Texture>(hud.tex_solid)
            .add_component<Components::TextureAtlas>(1, hud.src_h_px, 1, srcCols, srcCols)
            .add_component<Components::Color>(hud.fill_color_low)
            .remove_from_layer("default").add_to_layer(hud.layer.c_str());
          if (auto* atlas = col.get_component<Components::TextureAtlas>())
            atlas->drawIndex = cropL + i;
          hud.whiteSlices[i] = col;
        }

        if (hud.use_white_overlay) {
          if (!hud.tex_white) hud.tex_white = rect();
          hud.white_overlay = Engine::ECS::create_entity()
            .add_component<Components::Transform2D>(0.f, 0.f, 0.f, hud.width, hud.height)
            .add_component<Components::Texture>(hud.tex_white)
            .add_component<Components::Color>(Math::Vec4D(1, 1, 1, 1))
            .remove_from_layer("default").add_to_layer(hud.overlay_layer.c_str());
        }

        // Head on top layer
        hud.head = Engine::ECS::create_entity()
          .add_component<Components::Transform2D>(0.f, 0.f, 0.f, 120.f, 120.f)
          .add_component<Components::Texture>(hud.tex_head)
          .add_component<Components::Color>(Math::Vec4D(1, 1, 1, 1))
          .remove_from_layer("default").add_to_layer(hud.head_layer.c_str());
      }

      void place_white_columns(Components::HeartHUDSprite& hud, float backX, float backY, int visible) {
        const int cropL = std::clamp(hud.src_left_px, 0, hud.src_w_px - 1);
        const int cropR = std::clamp(hud.src_right_px, cropL + 1, hud.src_w_px);
        const int n = std::min((int)hud.whiteSlices.size(), cropR - cropL);

        // Band inside BarBack (measured from BarBack’s left)
        const float padX = hud.fill_inset_px.x, padY = hud.fill_inset_px.y;
        const float bandL = std::floor(backX + hud.fill_left_px + padX + 0.5f);
        const float bandR = std::floor(backX + hud.fill_right_px - padX + 0.5f);
        const float bandW = std::max(1.f, bandR - bandL);
        const float bandT = std::floor(backY + padY + 0.5f);
        const float bandH = std::round(std::max(1.f, hud.height - padY * 2.f));

        // Map ONLY the cropped pixels to the band
        const float scale = bandW / float(std::max(1, cropR - cropL));
        const int   shown = std::clamp(visible, 0, n);

        for (int i = 0; i < n; ++i) {
          const float L = std::round(bandL + i * scale);
          const float R = std::round(bandL + (i + 1) * scale);
          const float W = std::max(1.0f, R - L);
          const float CX = (L + R) * 0.5f;
          const bool  show = (i < shown);

          if (auto* tr = hud.whiteSlices[i].get_component<Components::Transform2D>()) {
            tr->position.x = CX;
            tr->position.y = bandT + bandH * 0.5f;
            tr->rotation = 0.f;
            tr->scale.x = W;
            tr->scale.y = show ? bandH : 0.f;
          }
        }
      }

      void promote_overlay_to_front(Components::HeartHUDSprite& hud) {
        if (!hud.use_white_overlay) return;
        if (!hud.white_overlay.is_valid()) return;
        hud.white_overlay.remove_from_layer(hud.overlay_layer.c_str());
        hud.white_overlay.add_to_layer(hud.overlay_layer.c_str());
      }

      void promote_head_to_front(Components::HeartHUDSprite& hud) {
        if (!hud.head.is_valid()) return;
        hud.head.remove_from_layer(hud.head_layer.c_str());
        hud.head.add_to_layer(hud.head_layer.c_str());
      }

    };

  }
}