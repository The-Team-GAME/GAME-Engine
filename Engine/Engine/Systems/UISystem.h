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

    // System to handle dynamic button collider updates
    class DynamicButtonSystem : public Engine::ECS::System {
    public:
      DynamicButtonSystem() {
        require_components<Components::DynamicButton, Components::Transform2D, Components::BoxCollider>();
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* dynamic_button = entity.get_component<Components::DynamicButton>();
        auto* box_collider = entity.get_component<Components::BoxCollider>();

        if (!dynamic_button || !box_collider) return;

        // TODO: very inefficient to initialize every frame, optimize later
        // Initialize DynamicButton with current collider values if not set
        bool needs_init = false;
        if (box_collider->type == Components::ColliderType::AABB) {
          needs_init = (dynamic_button->normal.width == 0.0f && dynamic_button->normal.height == 0.0f);
        }
        else {
          needs_init = dynamic_button->normal.vertices.empty();
        }

        if (needs_init) {
          if (box_collider->type == Components::ColliderType::AABB) {
            dynamic_button->normal.width = box_collider->width;
            dynamic_button->normal.height = box_collider->height;
            dynamic_button->hovered.width = box_collider->width;
            dynamic_button->hovered.height = box_collider->height;
            dynamic_button->clicked.width = box_collider->width;
            dynamic_button->clicked.height = box_collider->height;
          }
          else {
            dynamic_button->normal.vertices = box_collider->polygon.vertices;
            dynamic_button->hovered.vertices = box_collider->polygon.vertices;
            dynamic_button->clicked.vertices = box_collider->polygon.vertices;
          }
        }

        const auto mouse_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

        if (!mouse_opt.has_value()) {
          // Mouse not in viewport - ensure button is in normal state
          if (dynamic_button->is_hovered) {
            apply_normal_state(*dynamic_button, *box_collider);
            dynamic_button->is_hovered = false;
          }
          return;
        }

        const Math::Vec2D& mouse = mouse_opt.value();

        // Update collider position
        auto* tr = entity.get_component<Components::Transform2D>();
        update_collider_position(*box_collider, *tr, entity);

        // Check if mouse is inside
        bool inside = is_point_inside_collider(mouse, *box_collider);
        bool is_mouse_down = Engine::Input::is_pressed(KeyCode::MouseLeft);

        // State priority: Clicked > Hovered > Normal
        if (inside && is_mouse_down) {
          // Mouse is down on button - apply clicked state
          if (!dynamic_button->is_clicked) {
            apply_clicked_state(*dynamic_button, *box_collider);
            dynamic_button->is_clicked = true;
            dynamic_button->is_hovered = false;
          }
        }
        else if (inside && !is_mouse_down) {
          // Mouse is hovering but not clicked
          if (!dynamic_button->is_hovered || dynamic_button->is_clicked) {
            apply_hovered_state(*dynamic_button, *box_collider);
            dynamic_button->is_hovered = true;
            dynamic_button->is_clicked = false;
          }
        }
        else {
          // Mouse is outside - return to normal state
          if (dynamic_button->is_hovered || dynamic_button->is_clicked) {
            apply_normal_state(*dynamic_button, *box_collider);
            dynamic_button->is_hovered = false;
            dynamic_button->is_clicked = false;
          }
        }
      }

    private:
      void apply_normal_state(const Components::DynamicButton& button, Components::BoxCollider& collider) {
        if (collider.type == Components::ColliderType::AABB) {
          // Update AABB dimensions
          collider.width = button.normal.width;
          collider.height = button.normal.height;
          collider.aabb.half_width = button.normal.width / 2.0f;
          collider.aabb.half_height = button.normal.height / 2.0f;
        }
        else if (collider.type == Components::ColliderType::OBB ||
          collider.type == Components::ColliderType::POLYGON) {
          // Update polygon vertices
          if (!button.normal.vertices.empty()) {
            collider.polygon.vertices = button.normal.vertices;
          }
        }
      }

      void apply_hovered_state(const Components::DynamicButton& button, Components::BoxCollider& collider) {
        if (collider.type == Components::ColliderType::AABB) {
          // Update AABB dimensions
          collider.width = button.hovered.width;
          collider.height = button.hovered.height;
          collider.aabb.half_width = button.hovered.width / 2.0f;
          collider.aabb.half_height = button.hovered.height / 2.0f;
        }
        else if (collider.type == Components::ColliderType::OBB ||
          collider.type == Components::ColliderType::POLYGON) {
          // Update polygon vertices
          if (!button.hovered.vertices.empty()) {
            collider.polygon.vertices = button.hovered.vertices;
          }
        }
      }

      void apply_clicked_state(const Components::DynamicButton& button, Components::BoxCollider& collider) {
        if (collider.type == Components::ColliderType::AABB) {
          // Update AABB dimensions
          collider.width = button.clicked.width;
          collider.height = button.clicked.height;
          collider.aabb.half_width = button.clicked.width / 2.0f;
          collider.aabb.half_height = button.clicked.height / 2.0f;
        }
        else if (collider.type == Components::ColliderType::OBB ||
          collider.type == Components::ColliderType::POLYGON) {
          // Update polygon vertices
          if (!button.clicked.vertices.empty()) {
            collider.polygon.vertices = button.clicked.vertices;
          }
        }
      }

      void update_collider_position(Components::BoxCollider& collider,
        const Components::Transform2D& transform,
        Engine::ECS::Entity& entity) {
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

      bool is_point_inside_collider(const Math::Vec2D& point, const Components::BoxCollider& collider) {
        if (collider.type == Components::ColliderType::AABB) {
          Polygon poly(collider.aabb);
          const std::vector<Vec2D> vertices = poly.get_transformed_vertices();
          return point_in_polygon(point, vertices);
        }
        else {
          const std::vector<Vec2D> vertices = collider.polygon.get_transformed_vertices();
          return point_in_polygon(point, vertices);
        }
      }

      bool point_in_polygon(const Math::Vec2D& point, const std::vector<Vec2D>& vertices) {
        bool inside = false;
        size_t n = vertices.size();

        for (size_t i = 0, j = n - 1; i < n; j = i++) {
          const Vec2D& vi = vertices[i];
          const Vec2D& vj = vertices[j];

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