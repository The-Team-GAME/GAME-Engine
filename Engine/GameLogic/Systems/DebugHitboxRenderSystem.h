//-----------------------------------------------------------------------------------
// file:    DebugHitboxRenderSystem.h
// project: Monkey Stole My GPA
//
// brief:   System for rendering hitboxes (colliders) when the debug layer is active.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../Components/GameplayComponent.h"

namespace Engine {
  namespace Systems {

    class DebugHitboxRenderSystem : public Engine::ECS::System {
    public:
      DebugHitboxRenderSystem() {
        require_components<Components::BoxCollider, Components::Transform2D>();
      }

      void draw_entity(const Engine::ECS::Entity& entity) override {
        if (!Engine::ECS::is_layer_active("debug")) return;

        auto* transform = entity.get_component<Components::Transform2D>();
        auto* collider = entity.get_component<Components::BoxCollider>();


        if (!transform || !collider) return;

        // Determine color based on component types
        Math::Vec4D color(0.0f, 0.0f, 0.0f, 0.0f);

        if (entity.has_component<Components::Attack>()) {
          color = Math::Vec4D(1.0f, 0.0f, 0.0f, 0.2f); // Red (Attack)
        }
        else if (entity.has_component<Components::Player>() || entity.has_component<Components::EnemyHealth>()) {
          color = Math::Vec4D(0.0f, 1.0f, 0.0f, 0.2f); // Green (Hurtbox/Character)
        }

        // Calculate flipped offset
        Math::Vec2D flipped_offset = collider->offset;
        if (auto* flip = entity.get_component<Components::Flip>()) {
          flipped_offset.x *= flip->value.x;
          flipped_offset.y *= flip->value.y;
        }

        if (collider->type == Components::ColliderType::AABB) {
          Engine::Graphics::draw_rect(
            transform->position + flipped_offset,
            Math::Vec2D(collider->width, collider->height),
            0.0f,
            color
          );
        } else if (collider->type == Components::ColliderType::POLYGON) {
          // Draw polygon collider
          if (!collider->polygon.vertices.empty()) {
            std::vector<Math::Vec2D> world_vertices;
            for (const auto& v : collider->polygon.vertices) {
              // Apply flip to each vertex
              Math::Vec2D flipped_v = v;
              if (auto* flip = entity.get_component<Components::Flip>()) {
                flipped_v.x *= flip->value.x;
                flipped_v.y *= flip->value.y;
              }
              world_vertices.push_back(transform->position + flipped_offset + flipped_v);
            }
            
            // Draw edges
            for (size_t i = 0; i < world_vertices.size(); ++i) {
              size_t next = (i + 1) % world_vertices.size();
              Engine::Graphics::draw_line(world_vertices[i], world_vertices[next], color);
            }
          }
        }
      }
    };

  } // namespace Systems
} // namespace Engine
