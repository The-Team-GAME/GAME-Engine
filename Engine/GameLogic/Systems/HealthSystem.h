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

    class HealthWaveHUDSystem final : public Engine::ECS::System {
    public:
      HealthWaveHUDSystem() { require_components<Components::Heartrate, Components::HealthWaveHUD>(); }

      void update_entity(Engine::ECS::Entity& e) override {
        using namespace Components;

        auto* hr = e.get_component<Heartrate>();
        auto* hud = e.get_component<HealthWaveHUD>();
        if (!hr || !hud) return;

        ensure_wave(*hud);

        const float dt = Engine::Time::get_fixed_dt();

        // Normalize heartrate to [0..1] using low..crash_out
        const float bpm = std::max(1.f, hr->heartrate);
        const float base = hr->low;
        const float span = std::max(1e-3f, hr->crash_out - hr->low);
        const float target = std::clamp((bpm - base) / span, 0.f, 1.f);

        // Instant + delayed fill
        hud->front_ratio = target;
        if (hud->after_ratio > hud->front_ratio) {
          hud->after_ratio = std::max(hud->front_ratio, hud->after_ratio - hud->drain_speed * dt);
        }
        else {
          hud->after_ratio = std::min(hud->front_ratio, hud->after_ratio + hud->drain_speed * dt * 0.6f);
        }

        // Heartbeat animation (period from bpm)
        advance_heartbeat(*hud, bpm, dt);

        // Layout
        layout_wave(*hud);
      }

    private:
      static Engine::Graphics::Texture default_rect() { return Engine::Resources::get_rect_texture(); }

      void ensure_wave(Components::HealthWaveHUD& hud) {
        if (hud.initialized) return;
        hud.initialized = true;
        if (!hud.tex) hud.tex = default_rect();

        const int n = std::max(3, hud.segments);
        hud.back.resize(n);
        hud.after.resize(n);
        hud.front.resize(n);

        auto make_seg = [&](float thick, Components::HealthWaveHUD::RGBA c) {
          return Engine::ECS::create_entity()
            .add_component<Components::Transform2D>(0.f, 0.f, 0.f, thick, thick)
            .add_component<Components::Texture>(hud.tex)
            .add_component<Components::Color>(c.r, c.g, c.b, c.a)
            .remove_from_layer("default")
            .add_to_layer(hud.layer.c_str());
          };
        for (int i = 0; i < n; ++i) {
          hud.back[i] = make_seg(hud.back_thickness, hud.back_color);
          hud.after[i] = make_seg(hud.after_thickness, hud.after_color);
          hud.front[i] = make_seg(hud.front_thickness, hud.front_color);
        }
      }

      static inline float tri(float t) { return 2.f * std::fabs(2.f * (t - std::floor(t + 0.5f))) - 1.f; }

      void advance_heartbeat(Components::HealthWaveHUD& hud, float bpm, float dt) {
        const float period = (bpm > 1.f) ? (60.f / bpm) : 1.f;
        hud.beat_phase += dt / period;
        if (hud.beat_phase >= 1.f) hud.beat_phase -= 1.f;
        hud.spike_pos += dt / period;               // sweep left->right
        while (hud.spike_pos >= 1.f) hud.spike_pos -= 1.f;
      }

      // Smooth base: two sines blended
      inline void wave_point_base(const Components::HealthWaveHUD& hud, float t, float& x, float& y) {
        const float W = hud.width, H = hud.height;
        const float s1 = std::sin(6.28318f * (t * 1.2f));
        const float s2 = std::sin(6.28318f * (t * 3.4f)); // detail
        const float amp = H * 0.32f;
        const float yoff = H * 0.42f;                     // vertical bias inside band
        x = hud.anchor_px.x + t * W;
        y = hud.anchor_px.y + yoff + (s1 * 0.6f + s2 * 0.4f) * amp;
      }

      // ECG spike
      inline float wave_spike_offset(const Components::HealthWaveHUD& hud, float t) {
        const float w = std::max(0.005f, hud.spike_width_norm);
        const float d = std::fabs(t - hud.spike_pos);
        const float qrs = std::pow(std::max(0.f, 1.f - d / w), 6.f);
        return -qrs * hud.spike_height_px;
      }

      // Center + tangent angle
      inline void wave_point(const Components::HealthWaveHUD& hud, float t, float& x, float& y, float& ang) {
        float x0, y0, x1, y1;
        wave_point_base(hud, t, x0, y0); y0 += wave_spike_offset(hud, t);

        const float dt = 1.f / std::max(32, hud.segments * 2);
        const float t1 = std::min(1.f, t + dt);
        wave_point_base(hud, t1, x1, y1); y1 += wave_spike_offset(hud, t1);

        x = x0; y = y0;
        ang = std::atan2(y1 - y0, x1 - x0);
      }

      void layout_wave(Components::HealthWaveHUD& hud) {
        const int n = std::max(6, hud.segments);        // more segments for smoothness
        const int cnt_after = std::clamp(static_cast<int>(std::round(hud.after_ratio * n)), 0, n);
        const int cnt_front = std::clamp(static_cast<int>(std::round(hud.front_ratio * n)), 0, n);
        const float pulse = 1.f + hud.pulse_scale * std::exp(-8.f * hud.beat_phase);
        const float overlap = 1.25f;                    // slight overdraw to avoid gaps

        auto place_continuous = [&](std::vector<Engine::ECS::Entity>& layer, int visible, float thickness, float extraScale) {
          for (int i = 0; i < n; ++i) {
            if (!layer[i].is_valid()) continue;

            // segment span [t0, t1]
            const float t0 = (float)i / n;
            const float t1 = (float)(i + 1) / n;

            float x0b, y0b, x1b, y1b, a0, a1;
            // endpoints using the smooth wave + spike
            wave_point_base(hud, t0, x0b, y0b); y0b += wave_spike_offset(hud, t0);
            wave_point_base(hud, t1, x1b, y1b); y1b += wave_spike_offset(hud, t1);

            // center + angle from endpoints
            const float cx = (x0b + x1b) * 0.5f;
            const float cy = (y0b + y1b) * 0.5f;
            const float ang = std::atan2(y1b - y0b, x1b - x0b);
            const float len = std::hypot(x1b - x0b, y1b - y0b) * overlap;

            if (auto* tr = layer[i].get_component<Components::Transform2D>()) {
              tr->position.x = cx;
              tr->position.y = cy;
              tr->rotation = ang;               // align to tangent
              tr->scale.x = len * extraScale;  // LONG side along path
              tr->scale.y = (i < visible) ? (thickness * extraScale) : 0.f; // thickness; hide by 0 height
            }
          }
          };

        // Back outline: full ribbon
        place_continuous(hud.back, n, hud.back_thickness, 1.f);
        // After (delayed) and Front (current) with beat pulse scaling
        place_continuous(hud.after, cnt_after, hud.after_thickness, pulse * 0.98f);
        place_continuous(hud.front, cnt_front, hud.front_thickness, pulse);
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

        const float baseX = hud->anchor_px.x;
        const float baseY = hud->anchor_px.y;

        // Back & optional overlay & head (static)
        if (auto* t = hud->back.get_component<Components::Transform2D>()) {
          t->position.x = baseX + hud->width * 0.5f;
          t->position.y = baseY + hud->height * 0.5f;
          t->rotation = 0.f; t->scale.x = hud->width; t->scale.y = hud->height;
        }
        if (hud->use_white_overlay)
          if (auto* t = hud->white_overlay.get_component<Components::Transform2D>()) {
            t->position.x = baseX + hud->width * 0.5f;
            t->position.y = baseY + hud->height * 0.5f;
            t->rotation = 0.f; t->scale.x = hud->width; t->scale.y = hud->height;
          }
        if (auto* t = hud->head.get_component<Components::Transform2D>()) {
          t->position.x = baseX + 60.f; t->position.y = baseY + 58.f;
          t->rotation = 0.f; t->scale.x = 120.f; t->scale.y = 120.f;
        }

        const float bpm = std::max(1.f, hr->heartrate);
        const float span = std::max(1e-3f, hud->bpm_max - hud->bpm_min);
        const float ratio = std::clamp((bpm - hud->bpm_min) / span, 0.f, 1.f);
        const int   n = std::max(1, hud->slices);
        const int   vis = std::clamp(int(std::round(ratio * n)), 0, n);

        place_white_columns(*hud, baseX, baseY, vis);
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
          .add_component<Transform2D>(0.f, 0.f, 0.f, hud.width, hud.height)
          .add_component<Texture>(hud.tex_back)
          .add_component<Color>(Math::Vec4D(1, 1, 1, 1))
          .remove_from_layer("default").add_to_layer(hud.layer.c_str());

        // Build N column slices from WhiteSolidForFill.png using a texture atlas
        const int n = std::max(1, hud.slices);
        const int fw = std::max(1, hud.src_w_px / n);  // frame width in source tex pixels
        const int fh = std::max(1, hud.src_h_px);      // frame height

        hud.whiteSlices.resize(n);
        for (int i = 0; i < n; ++i) {
          auto e = Engine::ECS::create_entity()
            .add_component<Transform2D>(0.f, 0.f, 0.f, 1.f, 1.f)
            .add_component<Texture>(hud.tex_solid)
            .add_component<TextureAtlas>(fw, fh, 1, n, n) // rows=1, cols=n, frames=n
            .add_component<Color>(Math::Vec4D(1, 1, 1, 1))
            .remove_from_layer("default").add_to_layer(hud.layer.c_str());

          // Select the i-th column; no Animation used
          if (auto* atlas = e.get_component<TextureAtlas>()) {
            atlas->drawIndex = i; // <- pick sub-rect column
          }
          hud.whiteSlices[i] = e;
        }

        if (hud.use_white_overlay) {
          if (!hud.tex_white) hud.tex_white = rect();
          hud.white_overlay = Engine::ECS::create_entity()
            .add_component<Transform2D>(0.f, 0.f, 0.f, hud.width, hud.height)
            .add_component<Texture>(hud.tex_white)
            .add_component<Color>(Math::Vec4D(1, 1, 1, 1))
            .remove_from_layer("default").add_to_layer(hud.layer.c_str());
        }

        // Head badge
        hud.head = Engine::ECS::create_entity()
          .add_component<Transform2D>(0.f, 0.f, 0.f, 120.f, 120.f)
          .add_component<Texture>(hud.tex_head)
          .add_component<Color>(Math::Vec4D(1, 1, 1, 1))
          .remove_from_layer("default").add_to_layer(hud.layer.c_str());
      }

      void place_white_columns(Components::HeartHUDSprite& hud, float baseX, float baseY, int visible) {
        const int srcW = std::max(1, hud.src_w_px);
        const int n = std::min(std::max(1, hud.slices), (int)hud.whiteSlices.size());

        // We map source pixels -> HUD pixels with exact rounding to avoid drift
        const float hudLeft = std::floor(baseX + 0.5f);         // snap base to int
        const float hudTop = std::floor(baseY + 0.5f);
        const float hudW = std::round(hud.width);
        const float hudH = std::round(hud.height);
        const float scale = hudW / float(srcW);               // src px -> dst px

        const int shown = std::clamp(visible, 0, n);

        for (int i = 0; i < n; ++i) {
          // map column i from source space to dst pixel edges, snapped to ints
          const float L = std::round(hudLeft + i * scale);
          const float R = std::round(hudLeft + (i + 1) * scale);
          const float W = std::max(1.0f, R - L);
          const float CX = (L + R) * 0.5f;
          const bool  show = (i < shown);

          if (auto* tr = hud.whiteSlices[i].get_component<Components::Transform2D>()) {
            tr->position.x = CX;
            tr->position.y = hudTop + hudH * 0.5f;  // center vertically
            tr->rotation = 0.f;
            tr->scale.x = W;                     // exact integer width
            tr->scale.y = show ? hudH : 0.f;     // reveal by height
          }
        }
      }
    };

  }
}