//-----------------------------------------------------------------------------------
// file:    HealthSystems.h
// project: Monkey Stole My GPA
//
// author:   (100%)
//          Primary author
//
// email:   @digipen.edu
//
// brief:   This file defines a system that handles entity rotation by updating
//          rotation angles based on rotation speed and delta time in the ECS
//          framework.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once


#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Components/HealthComponents.h"
#include "../Time.h"

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
              const float e = t * t;
              float r = e, g = 0.1f, b = 1.0f - e, a = 1.0f;
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

  }
}