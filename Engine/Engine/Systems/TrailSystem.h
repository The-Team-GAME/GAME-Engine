//-----------------------------------------------------------------------------------
// file:    TrailSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the TrailSystem and FadeOutSystem, which manage the spawning and updating of trail effects and the fading of entities over time.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "../ECS/ECS.h"
#include "../Components/EffectComponents.h"
#include "../Components/GraphicsComponents.h"
#include "../Components/SpatialComponents.h"
#include "../Components/UtilityComponents.h"
#include "../Components/PhysicsComponents.h"
#include "../Time.h"

namespace Engine {
  namespace Systems {

    class TrailSystem : public Engine::ECS::System {
    public:
      TrailSystem() {
        require_components<Components::Trail, Components::Transform2D>();
      }

      void update_entity(Engine::ECS::Entity entity) override {
        auto* trail = entity.get_component<Components::Trail>();
        auto* transform = entity.get_component<Components::Transform2D>();

        f32 current_time = Engine::Time::get_time();
        if (current_time - trail->last_spawn_time >= trail->interval) {

          bool is_moving = true;
          if (entity.has_component<Components::RigidBody>()) {
            auto* rb = entity.get_component<Components::RigidBody>();
            // Check if velocity is significant (squared length > 100, i.e., length > 10)
            if ((rb->velocity.x * rb->velocity.x + rb->velocity.y * rb->velocity.y) < 100.0f) {
              is_moving = false;
            }
          }

          if (is_moving) {
            spawn_trail(entity, *trail, *transform);
            trail->last_spawn_time = current_time;
          }
        }
      }

      void spawn_trail(Engine::ECS::Entity source_entity, Components::Trail& trail, Components::Transform2D& transform) {
        auto trail_entity = Engine::ECS::create_entity();

        // Copy Transform
        trail_entity.add_component<Components::Transform2D>(transform);

        // Copy Texture if exists
        if (source_entity.has_component<Components::Texture>()) {
          trail_entity.add_component<Components::Texture>(*source_entity.get_component<Components::Texture>());
        }

        // Copy TextureAtlas if exists
        if (source_entity.has_component<Components::TextureAtlas>()) {
          trail_entity.add_component<Components::TextureAtlas>(*source_entity.get_component<Components::TextureAtlas>());
        }

        // Copy Flip if exists
        if (source_entity.has_component<Components::Flip>()) {
          trail_entity.add_component<Components::Flip>(*source_entity.get_component<Components::Flip>());
        }

        // Set Color (with start alpha)
        Math::Vec4D color = { 1.0f, 1.0f, 1.0f, trail.start_alpha };
        if (source_entity.has_component<Components::Color>()) {
          color = source_entity.get_component<Components::Color>()->to_vec4d();
          color.w = trail.start_alpha;
        }
        trail_entity.add_component<Components::Color>(color);

        // Add Lifetime
        trail_entity.add_component<Components::Lifetime>(trail.lifetime);

        // Add FadeOut
        trail_entity.add_component<Components::FadeOut>(trail.lifetime, trail.start_alpha, trail.end_alpha);

        // Add to player_trail layer
        trail_entity.clear_layers();
        trail_entity.add_to_layer("player_trail");
      }
    };

    class FadeOutSystem : public Engine::ECS::System {
    public:
      FadeOutSystem() {
        require_components<Components::FadeOut, Components::Color>();
      }

      void update_entity(Engine::ECS::Entity entity) override {
        auto* fade = entity.get_component<Components::FadeOut>();
        auto* color = entity.get_component<Components::Color>();

        f32 current_time = Engine::Time::get_time();
        f32 elapsed = current_time - fade->start_time;
        f32 t = elapsed / fade->duration;

        if (t > 1.0f) t = 1.0f;

        color->w = fade->start_alpha + (fade->end_alpha - fade->start_alpha) * t;
      }
    };

  }
}
