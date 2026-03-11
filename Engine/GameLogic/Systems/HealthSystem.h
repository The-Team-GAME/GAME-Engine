//-----------------------------------------------------------------------------------
// file:    HealthSystem.h
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//          Primary author
//
// email:   o.ziming@digipen.edu
//
// brief:   This file defines a ECS system that drive health gameplay logic and related HUD
//          visuals. Updates entity Health values with regen/drain, refreshes
//          the player HP UI, renders enemy rectangular HP bars with delayed
//          drain effect, and draws the animated screen-space heart-rate HUD
//          using the HeartHUDSprite configuration.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once


#include "pch.h"

#include "../Components/HealthComponents.h"

#include "../Components/EnemyHealthComponent.h"

#include "../../Engine/Graphics/PostProcessing.h"

namespace Engine {
  namespace Systems
  {
    // ---------------------------------------------------------------------------
    // DamageVignette: triggers a brief red vignette flash when the player is hit.
    // Call DamageVignette::trigger() from any damage callback. The HealthSystem
    // update() will automatically fade it out each frame.
    // ---------------------------------------------------------------------------
    namespace DamageVignette {
      inline float s_timer = 0.0f;  // seconds remaining
      inline float s_duration = 0.8f;  // total flash duration
      inline bool  s_was_enabled = false; // whether post-processing was on before we touched it

      inline void trigger(float duration = 0.8f) {
        // Save original post-processing enabled state on the first hit
        if (s_timer <= 0.0f) {
          s_was_enabled = Engine::Graphics::PostProcessing::get_settings().enabled;
        }
        s_duration = duration;
        s_timer = duration;  // restart timer on each hit
      }

      inline void update(float dt) {
        if (s_timer <= 0.0f) return;

        s_timer -= dt;

        auto& s = Engine::Graphics::PostProcessing::get_settings();
        if (s_timer > 0.0f) {
          // t goes from 1.0 (just hit) down to 0.0 (effect over)
          const float t = s_timer / s_duration;
          s.enabled = true;   // ensure pipeline runs
          s.vignette_enabled = true;
          s.vignette_color = glm::vec3(1.0f, 0.0f, 0.0f);
          s.vignette_intensity = t * 0.50f;   // max 0.50
          s.vignette_radius = 0.71f;
          s.vignette_smoothness = 0.50f;
          s.vignette_roundness = 1.0f;
        }
        else {
          // Effect finished – restore previous state
          s.vignette_enabled = false;
          s.vignette_intensity = 0.0f;
          s.enabled = s_was_enabled; // restore original enabled state
        }
      }
    } // namespace DamageVignette

    // Drives regen/drain and HUD for any entity with Components::Health
    class HealthSystem final : public Engine::ECS::System {
    public:
      HealthSystem() {
        require_components<Components::Health>();
      }

      // Tick damage vignette fade-out once per frame before processing entities.
      void update() override {
        DamageVignette::update(Engine::Time::get_fixed_dt());
      }

      void update_entity(Engine::ECS::Entity e) override {
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
          // Per-frame cleanup: keep only EnemyHealthBar sprites that are actually
          // referenced by some EnemyHealthBarRect component. Any other
          // EnemyHealthBar sprites (e.g. manually placed in the scene or
          // duplicated on reload) are destroyed.
          {
            std::unordered_set<Engine::ECS::EntityID> active_bar_ids;

            // Collect all bar sprite entities that are currently referenced by
            // EnemyHealthBarRect components.
            Engine::ECS::for_each_entity_with<Components::EnemyHealthBarRect>(
              [&](Engine::ECS::Entity owner)
              {
                if (auto* eb = owner.get_component<Components::EnemyHealthBarRect>()) {
                  if (eb->sprite.is_valid()) {
                    active_bar_ids.insert(eb->sprite.get_id());
                  }
                }
              }
            );

            // Destroy any EnemyHealthBar sprite that is not in the active set.
            Engine::ECS::for_each_entity_with<Components::StateMachine>(
              [&](Engine::ECS::Entity bar_entity)
              {
                auto* sm = bar_entity.get_component<Components::StateMachine>();
                if (!sm || sm->blueprint_id != "EnemyHealthBar") return;

                if (!active_bar_ids.contains(bar_entity.get_id())) {
                  bar_entity.destroy();
                }
              }
            );
          }
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
        bool is_player = e.has_component<Player>();
        // Don't destroy the player here — let the Death state machine handle it
        if (!is_player && hp->value <= 0) {
          e.destroy();
        }
      }
    };

    /*
    class EnemyHealthBarSystem : public Engine::ECS::System {
    public:
      EnemyHealthBarSystem() {
        require_components<Components::EnemyHealth, Components::Transform2D, Components::EnemyHealthBar>();
      }

      void update_entity(Engine::ECS::Entity e) override {
        using namespace Components;
        auto* hp = e.get_component<EnemyHealth>();
        auto* tr = e.get_component<Transform2D>();
        auto* bar = e.get_component<EnemyHealthBar>();
        if (!hp || !tr || !bar) return;

        ensure_bar(e, *bar);

        const float now = Engine::Time::get_time();
        const float dt = Engine::Time::get_fixed_dt();
        const int   cur = std::max(0, hp->current_hp);
        const int   mx = std::max(1, hp->max_hp);

        if (bar->last_hp < 0) { // first tick
          bar->last_hp = cur; bar->last_max = mx;
          bar->front_ratio = bar->after_ratio = float(cur) / mx;
        }

        // Damage
        if (cur < bar->last_hp) {
          const float newR = float(cur) / float(mx);
          const float oldR = float(bar->last_hp) / float(mx);
          bar->front_ratio = newR;           // snap
          bar->after_ratio = oldR;           // drain
          bar->has_been_hit = true;
          if (!bar->persist_after_first_hit) bar->visible_until = now + bar->show_duration;
        }
        else if (cur > bar->last_hp) {
          const float r = float(cur) / float(mx);
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
        else      hide_arc(*bar);

        if (hp->is_dead()) destroy_arc(*bar);
      }

    private:
      static Engine::Graphics::Texture rect() { return Engine::Resources::get_rect_texture(); }
      static inline float DEG(float d) { return d * 0.01745329251994329577f; }

      void ensure_bar(Engine::ECS::Entity, Components::EnemyHealthBar& bar) {
        using namespace Components;
        if (bar.initialized) return;
        bar.initialized = true;
        if (!bar.tex) bar.tex = rect();

        const int n = std::max(8, bar.segments);
        bar.base.resize(n);
        bar.after.resize(n);
        bar.front.resize(n);

        auto make_seg = [&](float thick, const EnemyHealthBar::RGBA& c) {
          auto seg = Engine::ECS::create_entity()
            .add_component<Components::Transform2D>(0.f, 0.f, 0.f, thick, thick)
            .add_component<Components::Texture>(bar.tex)
            .add_component<Components::Color>(c.r, c.g, c.b, c.a)
            .remove_from_layer("default").add_to_layer(bar.layer.c_str());
          return seg;
          };
        for (int i = 0; i < n; ++i) {
          bar.base[i] = make_seg(bar.base_thickness, bar.base_color);
          bar.after[i] = make_seg(bar.after_thickness, bar.after_color);
          bar.front[i] = make_seg(bar.front_thickness, bar.front_color);
        }

        // Caps
        bar.cap_left = Engine::ECS::create_entity()
          .add_component<Components::Transform2D>(0.f, 0.f, 0.f, bar.cap_len_px, bar.cap_thickness_px)
          .add_component<Components::Texture>(bar.tex)
          .add_component<Components::Color>(bar.cap_color.r, bar.cap_color.g, bar.cap_color.b, bar.cap_color.a)
          .remove_from_layer("default").add_to_layer(bar.layer.c_str());
        bar.cap_right = Engine::ECS::create_entity()
          .add_component<Components::Transform2D>(0.f, 0.f, 0.f, bar.cap_len_px, bar.cap_thickness_px)
          .add_component<Components::Texture>(bar.tex)
          .add_component<Components::Color>(bar.cap_color.r, bar.cap_color.g, bar.cap_color.b, bar.cap_color.a)
          .remove_from_layer("default").add_to_layer(bar.layer.c_str());
      }

      void hide_arc(Components::EnemyHealthBar& bar) {
        for (auto* layer : { &bar.base, &bar.after, &bar.front }) {
          for (auto& seg : *layer) if (auto* t = seg.get_component<Components::Transform2D>()) t->scale.y = 0.f;
        }
        for (auto cap : { bar.cap_left, bar.cap_right })
          if (auto* t = cap.get_component<Components::Transform2D>()) t->scale.y = 0.f;
      }

      void destroy_arc(Components::EnemyHealthBar& bar) {
        auto destroy_list = [&](std::vector<Engine::ECS::Entity>& v) { for (auto& e : v) if (e.is_valid()) e.destroy(); v.clear(); };
        destroy_list(bar.base);
        destroy_list(bar.after);
        destroy_list(bar.front);
        if (bar.cap_left.is_valid())  bar.cap_left.destroy();
        if (bar.cap_right.is_valid()) bar.cap_right.destroy();
        bar.initialized = false;
      }

      // Layout smooth arc
      void layout_arc(Engine::ECS::Entity owner, Components::EnemyHealthBar& bar) {
        using namespace Components;
        auto* to = owner.get_component<Components::Transform2D>();
        if (!to) return;

        const int n = std::max(8, bar.segments);

        // Arc ends (left to right on screen)
        float a0_deg, a1_deg;
        if (bar.use_center_span) {
          const float half = std::max(0.f, bar.arc_span_deg) * 0.5f;
          a0_deg = bar.center_angle_deg - half;
          a1_deg = bar.center_angle_deg + half;
        }
        else { a0_deg = bar.start_angle_deg; a1_deg = bar.end_angle_deg; }

        auto DEG = [](float d) { return d * 0.01745329251994329577f; };
        const float a0 = DEG(a0_deg), a1 = DEG(a1_deg);
        const float span = a1 - a0;

        const int vis_after = std::clamp(int(std::round(bar.after_ratio * n)), 0, n);
        const int vis_front = std::clamp(int(std::round(bar.front_ratio * n)), 0, n);

        const float cx0 = to->position.x + bar.offset.x;
        const float cy0 = to->position.y + bar.offset.y;

        // stronger overlap, no integer rounding
        const float overlap = 1.12f;   // to remove gaps
        auto place_ring = [&](std::vector<Engine::ECS::Entity>& ring,
          float thickness, int visible_from_right, bool show_full)
          {
            for (int i = 0; i < n; ++i) {
              if (!ring[i].is_valid()) continue;

              const float t0 = float(i) / n, t1 = float(i + 1) / n;
              const float ang0 = a0 + t0 * span, ang1 = a0 + t1 * span;

              const float x0 = cx0 + std::cos(ang0) * bar.radius;
              const float y0 = cy0 - std::sin(ang0) * bar.radius;
              const float x1 = cx0 + std::cos(ang1) * bar.radius;
              const float y1 = cy0 - std::sin(ang1) * bar.radius;

              const float cx = (x0 + x1) * 0.5f;
              const float cy = (y0 + y1) * 0.5f;
              const float ang = std::atan2(y1 - y0, x1 - x0);

              // elongate slice slightly to cover neighbors
              const float len = std::hypot(x1 - x0, y1 - y0) * overlap;

              // visibility
              const bool visible = show_full ? true : (i >= (n - visible_from_right));

              if (auto* tr = ring[i].get_component<Components::Transform2D>()) {
                tr->position.x = cx; tr->position.y = cy;
                tr->rotation = ang;
                tr->scale.x = len;
                tr->scale.y = visible ? thickness : 0.f;
              }
            }
          };

        // base to after to front
        place_ring(bar.base, bar.base_thickness, n, true);
        place_ring(bar.after, bar.after_thickness, vis_after, false);
        place_ring(bar.front, bar.front_thickness, vis_front, false);

        auto place_cap = [&](Engine::ECS::Entity cap, float end_deg, float slant_sign) {
          if (!cap.is_valid()) return;
          const float end = DEG(end_deg);
          const float x = cx0 + std::cos(end) * bar.radius;
          const float y = cy0 - std::sin(end) * bar.radius;

          const float tangent = end + 3.14159265359f * 0.5f; // along the arc
          const float rot = tangent + DEG(bar.cap_slant_deg * slant_sign);

          // push cap outward a hair so it sits cleanly on the edge
          const float outward = 0.5f;
          const float nx = std::cos(end);   // outward normal
          const float ny = -std::sin(end);

          if (auto* t = cap.get_component<Components::Transform2D>()) {
            t->position.x = x + nx * outward;
            t->position.y = y + ny * outward;
            t->rotation = rot;
            t->scale.x = bar.cap_len_px;
            t->scale.y = bar.cap_thickness_px;
          }
          };
        place_cap(bar.cap_left, a0_deg, +1.f);
        place_cap(bar.cap_right, a1_deg, -1.f);
      }
    };
    */

    class EnemyHealthBarRectSystem : public Engine::ECS::System {
    public:
      EnemyHealthBarRectSystem() {
        require_components<Components::EnemyHealth, Components::Transform2D, Components::EnemyHealthBarRect>();
      }

      void update_entity(Engine::ECS::Entity e) override {
        using namespace Components;
        // Per-frame cleanup: remove stray EnemyHealthBar sprites. However, if a
        // bar is already playing its Death animation, allow it to survive even
        // after the parent enemy is gone until that animation has finished.
        Engine::ECS::for_each_entity_with<Components::StateMachine>(
          [](Engine::ECS::Entity bar_entity)
          {
            auto* sm = bar_entity.get_component<Components::StateMachine>();
            if (!sm || sm->blueprint_id != "EnemyHealthBar") return;

            auto* anim = bar_entity.get_component<Components::Animation>();
            const bool death_playing =
              sm->get_current_state() == "Death" && anim && !anim->has_ended();

            // Once the Death animation has finished, this bar can be cleaned up.
            if (sm->get_current_state() == "Death" && (!anim || anim->has_ended())) {
              bar_entity.destroy();
              return;
            }

            auto* parent = bar_entity.get_component<Components::Parent>();
            if (!parent || parent->parent_id == Engine::ECS::INVALID_ENTITY) {
              if (!death_playing) {
                bar_entity.destroy();
              }
              return;
            }

            Engine::ECS::Entity parent_entity(parent->parent_id);
            if (!parent_entity.is_valid() ||
              !parent_entity.has_component<Components::EnemyHealth>()) {
              if (!death_playing) {
                bar_entity.destroy();
              }
            }
          }
        );

        auto* hp = e.get_component<EnemyHealth>();
        auto* tr = e.get_component<Transform2D>();
        auto* bar = e.get_component<EnemyHealthBarRect>();
        if (!hp || !tr || !bar) return;

        const float now = Engine::Time::get_time();

        const int cur = std::max(0, hp->current_hp);
        const int mx = std::max(1, hp->max_hp);

        ensure_bar(e, *bar, mx);

        if (bar->last_hp < 0) {
          bar->last_hp = cur;
          bar->last_max = mx;
          bar->visible_until = now + (bar->persist_after_first_hit ? 60.f : bar->show_duration);
        }
        else if (bar->last_max != mx) {
          destroy_rect(*bar);
          ensure_bar(e, *bar, mx);
          bar->last_max = mx;
        }

        if (cur < bar->last_hp) {
          bar->has_been_hit = true;
          if (!bar->persist_after_first_hit) bar->visible_until = now + bar->show_duration;
          restart_bar_animation(*bar);
        }
        else if (cur > bar->last_hp) {
          bar->visible_until = now + 0.4f;
        }
        bar->last_hp = cur;
        bar->last_max = mx;

        bool death_anim_active = false;
        if (bar->sprite.is_valid()) {
          if (auto* sm = bar->sprite.get_component<Components::StateMachine>()) {
            if (sm->get_current_state() == "Death") {
              if (auto* anim = bar->sprite.get_component<Components::Animation>()) {
                death_anim_active = !anim->has_ended();
              }
            }
          }
        }

        const bool should_show_bar = death_anim_active ||
          (bar->has_been_hit &&
            (bar->persist_after_first_hit || now <= bar->visible_until));

        if (should_show_bar) {
          layout_rect(e, *tr, *bar, cur, mx);
        }
        else {
            if (bar->sprite.is_valid()) {
              if (auto* sm = bar->sprite.get_component<Components::StateMachine>()) {
                if (sm->get_current_state() != "Blank") {
                  sm->set_state_immediate("Blank", bar->sprite);
                }
              }
            }
            hide_rect(*bar);
        }

        // When the enemy dies, play the health bar's "Death" animation
        // fully before destroying the bar sprite.
        if (hp->is_dead()) {
          if (bar->sprite.is_valid()) {
            if (auto* sm = bar->sprite.get_component<Components::StateMachine>()) {
              if (sm->get_current_state() != "Death") {
                // Switch immediately so the Death animation is applied this
                // frame and cannot be skipped by system ordering.
                sm->set_state_immediate("Death", bar->sprite);
                return;
              }
            }

            // Already in Death state: wait until the Death animation finishes
            // before we destroy the bar.
            if (auto* anim = bar->sprite.get_component<Components::Animation>()) {
              if (!anim->has_ended()) {
                return;
              }
            }
          }

          // Either no sprite/state machine, or Death animation has ended:
          // safe to destroy the bar now.
          destroy_rect(*bar);
        }
      }

    private:
      void ensure_bar(Engine::ECS::Entity owner, Components::EnemyHealthBarRect& bar, [[maybe_unused]] int max_hp) {
        using namespace Components;

        if (bar.initialized && bar.sprite.is_valid()) return;

        destroy_rect(bar);

        bar.initialized = true;

        auto prefab = Engine::Resources::load_prefab("prefabs/Enemy_HealthBar.json");
        bar.sprite = Engine::ECS::instantiate_prefab(prefab);

        if (bar.sprite.has_component<Components::UI>()) {
          bar.sprite.remove_component<Components::UI>();
        }

        if (auto* parent = bar.sprite.get_component<Components::Parent>()) {
          parent->parent_id = owner.get_id();
          parent->local_offset = Math::Vec2D(0.0f, 0.0f);
          if (auto* owner_transform = owner.get_component<Components::Transform2D>()) {
            parent->prev_parent_pos = owner_transform->position;
          }
          else {
            parent->prev_parent_pos = Math::Vec2D(0.0f, 0.0f);
          }
          parent->offset_initialized = true;
        }
        else {
          bar.sprite.add_component<Components::Parent>(owner.get_id(), Math::Vec2D(0.0f, 0.0f));
        }

        if (auto* transform = bar.sprite.get_component<Components::Transform2D>()) {
          const float square_size = std::max(bar.width, bar.height);
          if (transform->scale.x <= 0.0f || transform->scale.y <= 0.0f) {
            transform->scale.x = square_size;
            transform->scale.y = square_size;
          }
        }

        if (auto* color = bar.sprite.get_component<Components::Color>()) {
          color->a = 0.0f;
        }
        else {
          bar.sprite.add_component<Components::Color>(1.f, 1.f, 1.f, 0.f);
        }

        bar.sprite.remove_from_layer("default");
        bar.sprite.add_to_layer(bar.layer.c_str());

        if (auto* state_machine = bar.sprite.get_component<Components::StateMachine>()) {
          state_machine->blueprint_id = "EnemyHealthBar";
          state_machine->current_state_name = "Blank";
          state_machine->set_state_immediate("Blank", bar.sprite);
        }
        else {
          bar.sprite.add_component<Components::StateMachine>();
          if (auto* new_state_machine = bar.sprite.get_component<Components::StateMachine>()) {
            new_state_machine->blueprint_id = "EnemyHealthBar";
            new_state_machine->current_state_name = "Blank";
            new_state_machine->set_state_immediate("Blank", bar.sprite);
          }
        }

        restart_bar_animation(bar);
        hide_rect(bar);
      }

      void hide_rect(Components::EnemyHealthBarRect& bar) {
        if (auto* t = bar.sprite.get_component<Components::Transform2D>()) {
          t->scale.x = 0.f;
          t->scale.y = 0.f;
        }

        if (auto* color = bar.sprite.get_component<Components::Color>()) {
          color->a = 0.f;
        }
      }

      void destroy_rect(Components::EnemyHealthBarRect& bar) {
        if (bar.sprite.is_valid()) {
          bar.sprite.destroy();
        }
        bar.sprite = Engine::ECS::Entity();
        bar.initialized = false;
      }

      void restart_bar_animation(Components::EnemyHealthBarRect& bar) {
        if (auto* animation = bar.sprite.get_component<Components::Animation>()) {
          animation->startTime = Engine::Time::get_time();
        }

        if (auto* atlas = bar.sprite.get_component<Components::TextureAtlas>()) {
          atlas->drawIndex = 0;
        }
      }

      void layout_rect(Engine::ECS::Entity owner, const Components::Transform2D& to,
        Components::EnemyHealthBarRect& bar, [[maybe_unused]] int current_hp, [[maybe_unused]] int max_hp) {
        if (!bar.sprite.is_valid()) {
          return;
        }

        const float square_size = std::max(bar.width, bar.height);

        float anchor_x = to.position.x;
        float anchor_y = to.position.y;

        if (auto* collider = owner.get_component<Components::BoxCollider>()) {
          anchor_x += collider->offset.x;
          anchor_y += collider->offset.y - collider->height * 0.5f;
        }

        if (auto* t = bar.sprite.get_component<Components::Transform2D>()) {
          t->position.x = anchor_x + bar.offset.x;
          t->position.y = anchor_y + bar.offset.y;
          t->rotation = 0.f;

          if (t->scale.x <= 0.f || t->scale.y <= 0.f) {
            t->scale.x = square_size;
            t->scale.y = square_size;
          }
        }

        if (auto* color = bar.sprite.get_component<Components::Color>()) {
          color->a = 1.f;
        }
      }
    };


    class HeartHUDSpriteSystem final : public Engine::ECS::System {
    public:
      HeartHUDSpriteSystem() {
        require_components<Components::Health, Components::HeartHUDSprite>();
      }

      void update_entity(Engine::ECS::Entity e) override {
        using namespace Components;
        auto* hp = e.get_component<Health>();
        auto* hud = e.get_component<HeartHUDSprite>();
        if (!hp || !hud) return;

        if (!hud->screen_space) {
          ensure_created(*hud);

          const float headX = hud->anchor_px.x + hud->head_offset_px.x;
          const float headY = hud->anchor_px.y + hud->head_offset_px.y;
          const float backX = hud->anchor_px.x + hud->back_offset_px.x;
          const float backY = hud->anchor_px.y + hud->back_offset_px.y;

          // Back
          if (auto* t = hud->back.get_component<Components::Transform2D>()) {
            t->position.x = backX + hud->width * 0.5f;
            t->position.y = backY + hud->height * 0.5f;
            t->rotation = 0.f;
            t->scale.x = hud->width;
            t->scale.y = hud->height;
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
              t->rotation = 0.f;
              t->scale.x = hud->width;
              t->scale.y = hud->height;
            }
          }

          // Head
          if (auto* t = hud->head.get_component<Components::Transform2D>()) {
            t->position.x = headX + 60.f;
            t->position.y = headY + 58.f;
            t->rotation = 0.f;
            t->scale.x = 120.f;
            t->scale.y = 120.f;
          }

          // HP to ratio for world-space columns
          const float ratio = (hp->max > 0) ? std::clamp(static_cast<float>(hp->value) / static_cast<float>(hp->max), 0.f, 1.f) : 0.f;

          if (hud->use_bpm_tint) {
            Math::Vec4D c;
            c.x = hud->fill_color_low.x + (hud->fill_color_high.x - hud->fill_color_low.x) * ratio;
            c.y = hud->fill_color_low.y + (hud->fill_color_high.y - hud->fill_color_low.y) * ratio;
            c.z = hud->fill_color_low.z + (hud->fill_color_high.z - hud->fill_color_low.z) * ratio;
            c.w = hud->fill_color_low.w + (hud->fill_color_high.w - hud->fill_color_low.w) * ratio;
            for (auto& s : hud->whiteSlices)
              if (auto* col = s.get_component<Color>()) *col = c;
          }

          const int n = std::max(1, hud->slices);
          const int vis = std::clamp(int(std::round(ratio * n)), 0, n);

          place_white_columns(*hud, backX, backY, vis);
          promote_overlay_to_front(*hud);
          promote_head_to_front(*hud);

          return;
        }

      }

      // ---- UI draw path (screen-space, no camera) ----
      void draw_entity(const Engine::ECS::Entity e) override {
        using namespace Components;

        auto* hp = e.get_component<Health>();
        auto* hud = e.get_component<HeartHUDSprite>();
        if (!hp || !hud) return;

        if (!hud->screen_space) {
          // World-space HUD is handled by update_entity + normal render system
          return;
        }

        // Effective anchors in screen pixels
        const float headX = hud->anchor_px.x + hud->head_offset_px.x;
        const float headY = hud->anchor_px.y + hud->head_offset_px.y;
        const float backX = hud->anchor_px.x + hud->back_offset_px.x;
        const float backY = hud->anchor_px.y + hud->back_offset_px.y;

        // Background bar (screen-space)
        Engine::Graphics::draw_texture_ui(
          hud->tex_back,
          Math::Vec2D(backX + hud->width * 0.5f, backY + hud->height * 0.5f),
          Math::Vec2D(hud->width, hud->height),
          0.0f,
          Math::Vec4D(1.f, 1.f, 1.f, 1.f)
        );

        // HP ratio 0..1
        const float ratio = (hp->max > 0) ? std::clamp(static_cast<float>(hp->value) / static_cast<float>(hp->max), 0.f, 1.f) : 0.f;

        // BPM-driven tint
        Math::Vec4D fill_color = hud->fill_color_low;
        if (hud->use_bpm_tint) {
          fill_color.x = hud->fill_color_low.x + (hud->fill_color_high.x - hud->fill_color_low.x) * ratio;
          fill_color.y = hud->fill_color_low.y + (hud->fill_color_high.y - hud->fill_color_low.y) * ratio;
          fill_color.z = hud->fill_color_low.z + (hud->fill_color_high.z - hud->fill_color_low.z) * ratio;
          fill_color.w = hud->fill_color_low.w + (hud->fill_color_high.w - hud->fill_color_low.w) * ratio;
        }

        // Number of visible columns based on ratio
        const int cropL = std::clamp(hud->src_left_px, 0, hud->src_w_px - 1);
        const int cropR = std::clamp(hud->src_right_px, cropL + 1, hud->src_w_px);
        const int total_cols = std::max(1, cropR - cropL);
        const int visible_cols = std::clamp(int(std::round(ratio * total_cols)), 0, total_cols);

        // Draw the white jagged band in screen-space using sub-textures
        draw_white_columns_ui(*hud, backX, backY, cropL, cropR, visible_cols, fill_color);

        // Overlay on top
        if (hud->use_white_overlay) {
          const float baseX = hud->overlay_relative_to_back ? backX : hud->anchor_px.x;
          const float baseY = hud->overlay_relative_to_back ? backY : hud->anchor_px.y;
          const float ovX = baseX + hud->overlay_offset_px.x;
          const float ovY = baseY + hud->overlay_offset_px.y;

          Engine::Graphics::draw_texture_ui(
            hud->tex_white,
            Math::Vec2D(ovX + hud->width * 0.5f, ovY + hud->height * 0.5f),
            Math::Vec2D(hud->width, hud->height),
            0.0f,
            Math::Vec4D(1.f, 1.f, 1.f, 1.f)
          );
        }

        // Head on top-left
        Engine::Graphics::draw_texture_ui(
          hud->tex_head,
          Math::Vec2D(headX + 60.f, headY + 58.f),
          Math::Vec2D(120.f, 120.f),
          0.0f,
          Math::Vec4D(1.f, 1.f, 1.f, 1.f)
        );
      }

    private:
      static Engine::Graphics::Texture rect() { return Engine::Resources::get_rect_texture(); }

      // ---- existing world-space helpers ----
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

        // --- use low color when creating each slice ---
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

        const float padX = hud.fill_inset_px.x, padY = hud.fill_inset_px.y;
        const float bandL = std::floor(backX + hud.fill_left_px + padX + 0.5f);
        const float bandR = std::floor(backX + hud.fill_right_px - padX + 0.5f);
        const float bandW = std::max(1.f, bandR - bandL);
        const float bandT = std::floor(backY + padY + 0.5f);
        const float bandH = std::round(std::max(1.f, hud.height - padY * 2.f));

        const float scale = bandW / float(std::max(1, cropR - cropL));
        const int   shown = std::clamp(visible, 0, n);

        for (int i = 0; i < n; ++i) {
          const float L = std::round(bandL + i * scale);
          const float R = std::round(bandL + (i + 1) * scale);
          const float W = std::max(1.0f, R - L);
          const float CX = (L + R) * 0.5f;
          const bool show = (i < shown);

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

      // helper for screen-space white band
      void draw_white_columns_ui(const Components::HeartHUDSprite& hud,
        float backX, float backY,
        int cropL, int cropR,
        int visible_cols,
        const Math::Vec4D& color) {
        const int totalCols = std::max(1, cropR - cropL);
        const int shown = std::clamp(visible_cols, 0, totalCols);

        const float padX = hud.fill_inset_px.x, padY = hud.fill_inset_px.y;
        const float bandL = std::floor(backX + hud.fill_left_px + padX + 0.5f);
        const float bandR = std::floor(backX + hud.fill_right_px - padX + 0.5f);
        const float bandW = std::max(1.f, bandR - bandL);
        const float bandT = std::floor(backY + padY + 0.5f);
        const float bandH = std::round(std::max(1.f, hud.height - padY * 2.f));

        const float scale = bandW / float(std::max(1, cropR - cropL));

        for (int i = 0; i < totalCols; ++i) {
          if (i >= shown) break;

          const float L = std::round(bandL + i * scale);
          const float R = std::round(bandL + (i + 1) * scale);
          const float W = std::max(1.0f, R - L);
          const float CX = (L + R) * 0.5f;

          const int col = cropL + i;
          const float u0 = float(col) / float(hud.src_w_px);
          const float u1 = float(col + 1) / float(hud.src_w_px);

          std::array<Math::Vec2D, 4> tex_coords = {
            Math::Vec2D(u0, 1.0f),
            Math::Vec2D(u1, 1.0f),
            Math::Vec2D(u1, 0.0f),
            Math::Vec2D(u0, 0.0f)
          };

          Engine::Graphics::draw_subtexture_ui(
            hud.tex_solid,
            tex_coords,
            Math::Vec2D(CX, bandT + bandH * 0.5f),
            Math::Vec2D(W, bandH),
            0.0f,
            color
          );
        }
      }
    };

  }
}