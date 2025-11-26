//-----------------------------------------------------------------------------------
// file:    UISystem.h
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (40%)
//          Primary author
// 
//          Ryan Ang (60%)
//          Secondary author
//
// email:   o.ziming@digipen.edu
//
// brief:   This file defines a Event-driven Clickable System that checks for entities
//          that has the clickable component and gets the mouse position of when the
//          object was clicked. (Ryan) It also handles the dynamic scaling animation when the mouse
//          hovers over the clickable entity.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------


#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../InputManager.h"

namespace Engine
{
  namespace Systems
  {

    class ClickableSystem : public Engine::ECS::System {
    public:
      ClickableSystem() {
        require_components<Components::Clickable, Components::Transform2D, Components::BoxCollider>();
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* clickable = entity.get_component<Components::Clickable>();
        auto* tr = entity.get_component<Components::Transform2D>();
        auto* box_collider = entity.get_component<Components::BoxCollider>();

        if (!clickable || !tr || !box_collider) return;

        const auto mouse_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

        if (!mouse_opt.has_value()) {
          return;
        }

        const Math::Vec2D& mouse = mouse_opt.value();

        // Update the collider position before checking collision
        update_collider_position(*box_collider, *tr, entity);

        // Use precise collision detection based on collider type
        bool inside = is_point_inside_collider(mouse, *box_collider, *tr);

        // mouse click detection
        if (inside) {
          clickable->on_hover(entity);

          if (Engine::Input::is_pressed(KeyCode::MouseLeft)) {
            clickable->on_click();
          }
        }
        else {
          clickable->exit_hover(entity);
        }
      }

    private:
      // Update collider position to match transform
      void update_collider_position(Components::BoxCollider& collider,
        const Components::Transform2D& transform,
        Engine::ECS::Entity& entity) {
        // Calculate flipped offset
        Vec2D flipped_offset = collider.offset;
        if (auto* flip = entity.get_component<Components::Flip>()) {
          flipped_offset.x *= flip->value.x;
          flipped_offset.y *= flip->value.y;
        }

        if (collider.type == Components::ColliderType::AABB) {
          collider.aabb.pos = transform.position + flipped_offset;
        }
        else if (collider.type == Components::ColliderType::OBB ||
          collider.type == Components::ColliderType::POLYGON) {
          collider.polygon.position = transform.position + flipped_offset;
          collider.polygon.rotation = transform.rotation;
        }
      }
      // Point-in-polygon test for any collider type
      bool is_point_inside_collider(const Math::Vec2D& point, const Components::BoxCollider& collider, const Components::Transform2D& transform) {

        if (collider.type == Components::ColliderType::AABB) {
          // Convert AABB to polygon and get transformed vertices
          Polygon poly(collider.aabb);
          const std::vector<Vec2D> vertices = poly.get_transformed_vertices();
          return point_in_polygon(point, vertices);
        }
        else {
          // Polygon collision (works for OBB and POLYGON types)
          // Use point-in-polygon ray casting algorithm
          const std::vector<Vec2D> vertices = collider.polygon.get_transformed_vertices();
          return point_in_polygon(point, vertices);
        }
      }

      // Ray casting algorithm for point-in-polygon test
      bool point_in_polygon(const Math::Vec2D& point, const std::vector<Vec2D>& vertices) {
        bool inside = false;
        size_t n = vertices.size();

        for (size_t i = 0, j = n - 1; i < n; j = i++) {
          const Vec2D& vi = vertices[i];
          const Vec2D& vj = vertices[j];

          // Check if point is on horizontal ray crossing
          if (((vi.y > point.y) != (vj.y > point.y)) &&
            (point.x < (vj.x - vi.x) * (point.y - vi.y) / (vj.y - vi.y) + vi.x)) {
            inside = !inside;
          }
        }

        return inside;
      }
    };

  } // namespace Systems
} // namespace Engine