//-----------------------------------------------------------------------------------
// file:    GizmoUtils.h
// project: Monkey Stole My GPA
//
// brief:   Shared utility functions and constants for the gizmo systems.
//          Consolidates math helpers, draw dispatchers, AABB computation, and
//          hit-testing that were previously duplicated across multiple systems.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../InputManager.h"
#include "../Graphics/Renderer.h"
#include <cfloat>
#include <cmath>
#include <algorithm>

namespace Engine {
namespace Systems {
namespace GizmoUtil {

  // ====================== Constants ======================

  static constexpr float kEpsilon           = 0.0001f;
  static constexpr float kMinScale          = 4.0f;
  static constexpr float kSnapAngle         = 15.0f;
  static constexpr float kOutlineThickness  = 2.0f;
  static constexpr float kWorldOutlineThick = 2.5f;
  static constexpr float kHandleSize        = 10.0f;
  static constexpr float kPi                = 3.14159265358979323846f;

  // ====================== Math Helpers ======================

  inline bool point_in_rect(const Math::Vec2D& p, const Math::Vec2D& c, const Math::Vec2D& size) {
    const float hw = size.x * 0.5f, hh = size.y * 0.5f;
    return (p.x >= c.x - hw) && (p.x <= c.x + hw) && (p.y >= c.y - hh) && (p.y <= c.y + hh);
  }

  inline float dist_to_segment(const Math::Vec2D& p, const Math::Vec2D& a, const Math::Vec2D& b) {
    const float vx = b.x - a.x, vy = b.y - a.y;
    const float wx = p.x - a.x, wy = p.y - a.y;
    const float vv = vx * vx + vy * vy;
    float t = vv > 0.f ? (wx * vx + wy * vy) / vv : 0.f;
    t = std::max(0.f, std::min(1.f, t));
    const float px = a.x + t * vx, py = a.y + t * vy;
    const float dx = p.x - px, dy = p.y - py;
    return std::sqrt(dx * dx + dy * dy);
  }

  inline Math::Vec2D rot_deg(const Math::Vec2D& p, const Math::Vec2D& pivot, float deg) {
    const float r = deg * kPi / 180.0f;
    const float c = std::cos(r), s = std::sin(r);
    const float x = p.x - pivot.x, y = p.y - pivot.y;
    return { pivot.x + (x * c - y * s), pivot.y + (x * s + y * c) };
  }

  inline float distance(const Math::Vec2D& a, const Math::Vec2D& b) {
    const float dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
  }

  inline float dot(const Math::Vec2D& a, const Math::Vec2D& b) {
    return a.x * b.x + a.y * b.y;
  }

  inline Math::Vec2D mul_add(const Math::Vec2D& a, float k, const Math::Vec2D& b) {
    return { a.x + k * b.x, a.y + k * b.y };
  }

  inline bool point_near(const Math::Vec2D& p, const Math::Vec2D& c, float r) {
    float dx = p.x - c.x, dy = p.y - c.y;
    return (dx * dx + dy * dy) <= r * r;
  }

  // ====================== Draw Dispatchers ======================
  // Automatically choose UI (screen-space) or world-space draw functions.

  inline void draw_rect_outline_auto(bool is_ui, const Math::Vec2D& pos, const Math::Vec2D& size,
                                     f32 rot, f32 thick, const Math::Vec4D& col) {
    if (is_ui) Graphics::draw_rect_outline_ui(pos, size, rot, thick, col);
    else       Graphics::draw_rect_outline(pos, size, rot, thick, col);
  }

  inline void draw_circle_outline_auto(bool is_ui, const Math::Vec2D& pos, f32 radius,
                                       f32 thick, const Math::Vec4D& col, int segments = 32) {
    if (is_ui) Graphics::draw_circle_outline_ui(pos, radius, thick, col, segments);
    else       Graphics::draw_circle_outline(pos, radius, thick, col, segments);
  }

  inline void draw_line_auto(bool is_ui, const Math::Vec2D& start, const Math::Vec2D& end,
                             f32 thick, const Math::Vec4D& col) {
    if (is_ui) Graphics::draw_line_ui(start, end, thick, col);
    else       Graphics::draw_line(start, end, thick, col);
  }

  // ====================== AABB Computation ======================

  struct AABBResult {
    Math::Vec2D min{ FLT_MAX, FLT_MAX };
    Math::Vec2D max{ -FLT_MAX, -FLT_MAX };
    bool all_ui = true;
    bool valid  = false;
  };

  // Rotation-aware AABB: rotates each entity's 4 corners by its rotation,
  // then takes the axis-aligned bounding box of all rotated corners.
  template<typename EntityIDContainer>
  inline AABBResult compute_aabb(const EntityIDContainer& ents) {
    AABBResult r;
    for (auto eid : ents) {
      Engine::ECS::Entity e(eid);
      auto* t = e.get_component<Components::Transform2D>();
      if (!t) continue;
      if (!e.has_component<Components::UI>()) r.all_ui = false;

      const float hw = t->scale.x * 0.5f, hh = t->scale.y * 0.5f;

      // If the entity has rotation, compute rotated corner positions
      if (std::abs(t->rotation) > kEpsilon) {
        const Math::Vec2D corners[4] = {
          rot_deg({ t->position.x - hw, t->position.y - hh }, t->position, t->rotation),
          rot_deg({ t->position.x + hw, t->position.y - hh }, t->position, t->rotation),
          rot_deg({ t->position.x + hw, t->position.y + hh }, t->position, t->rotation),
          rot_deg({ t->position.x - hw, t->position.y + hh }, t->position, t->rotation),
        };
        for (auto& c : corners) {
          r.min.x = std::min(r.min.x, c.x);
          r.min.y = std::min(r.min.y, c.y);
          r.max.x = std::max(r.max.x, c.x);
          r.max.y = std::max(r.max.y, c.y);
        }
      } else {
        r.min.x = std::min(r.min.x, t->position.x - hw);
        r.min.y = std::min(r.min.y, t->position.y - hh);
        r.max.x = std::max(r.max.x, t->position.x + hw);
        r.max.y = std::max(r.max.y, t->position.y + hh);
      }
    }
    r.valid = (r.min.x <= r.max.x);
    return r;
  }

  inline Math::Vec2D aabb_center(const AABBResult& r) {
    return { (r.min.x + r.max.x) * 0.5f, (r.min.y + r.max.y) * 0.5f };
  }

  inline Math::Vec2D aabb_size(const AABBResult& r) {
    return { r.max.x - r.min.x, r.max.y - r.min.y };
  }

  // ====================== Hit Testing ======================

  // Rotation-aware point-in-OBB: rotates mouse into entity's local space.
  inline bool is_point_in_entity(const Math::Vec2D& mouse, const Components::Transform2D* tr) {
    // Rotate mouse into the entity's local coordinate space
    Math::Vec2D local = rot_deg(mouse, tr->position, -tr->rotation);
    const float hw = tr->scale.x * 0.5f, hh = tr->scale.y * 0.5f;
    return (local.x >= tr->position.x - hw) && (local.x <= tr->position.x + hw) &&
           (local.y >= tr->position.y - hh) && (local.y <= tr->position.y + hh);
  }

  // Get correct mouse position for an entity (screen-space for UI, world-space for others).
  inline std::optional<Math::Vec2D> get_mouse_for_entity(const Engine::ECS::Entity& entity) {
    return entity.has_component<Components::UI>()
      ? Engine::Input::get_mouse_position_if_in_viewport()
      : Engine::Input::get_mouse_world_position_if_in_viewport();
  }

  // Check if the mouse is over any entity that already has a Gizmo component.
  // Used by selection systems to avoid re-selecting when clicking an already-selected entity.
  inline bool is_any_gizmo_entity_under_mouse() {
    bool found = false;
    Engine::ECS::for_each_entity_with<Components::Gizmo>([&](Engine::ECS::Entity gizmo_entity) {
      if (found) return;
      auto* tr = gizmo_entity.get_component<Components::Transform2D>();
      if (!tr) return;
      const auto mouse_opt = get_mouse_for_entity(gizmo_entity);
      if (!mouse_opt.has_value()) return;
      if (is_point_in_entity(mouse_opt.value(), tr)) found = true;
    });
    return found;
  }

  // Check if the mouse is over a specific entity (rotation-aware OBB test).
  inline bool is_entity_under_mouse(Engine::ECS::Entity entity) {
    auto* tr = entity.get_component<Components::Transform2D>();
    if (!tr) return false;
    const auto mouse_opt = get_mouse_for_entity(entity);
    if (!mouse_opt.has_value()) return false;
    return is_point_in_entity(mouse_opt.value(), tr);
  }

  // Check if mouse is over any entity in a set, using correct coordinate space per entity.
  template<typename EntityIDContainer>
  inline bool is_any_entity_under_mouse(const EntityIDContainer& ents,
                                        const Math::Vec2D& mouse_screen,
                                        const Math::Vec2D& mouse_world) {
    for (auto eid : ents) {
      Engine::ECS::Entity e(eid);
      auto* t = e.get_component<Components::Transform2D>();
      if (!t) continue;
      auto m = e.has_component<Components::UI>() ? mouse_screen : mouse_world;
      if (is_point_in_entity(m, t)) return true;
    }
    return false;
  }

  // ====================== Unified Bounding Rect Drawing ======================

  inline void draw_bounding_rect(const AABBResult& aabb, const Math::Vec4D& color) {
    if (!aabb.valid) return;
    Math::Vec2D center = aabb_center(aabb);
    Math::Vec2D size   = aabb_size(aabb);
    float thick = aabb.all_ui ? kOutlineThickness : kWorldOutlineThick;
    draw_rect_outline_auto(aabb.all_ui, center, size, 0.0f, thick, color);
  }

} // namespace GizmoUtil
} // namespace Systems
} // namespace Engine
