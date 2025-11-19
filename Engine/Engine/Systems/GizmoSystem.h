//-----------------------------------------------------------------------------------
// file:    GizmoSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of the gizmo system for entity selection and manipulation
//          in the game editor. Provides mouse-based selection of entities, visual
//          feedback for selected entities, and drag-and-drop repositioning.
//          Integrates with the input system and event manager for seamless
//          editor interaction.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../InputManager.h"
#include "../Graphics/Renderer.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include <imgui.h>

// TODO: M2: NEED TO WORK WITH THINGS WITH RIGIDBODY

namespace Engine
{
  namespace Systems
  {
    // System to add Gizmo component when entities with Transform2D are clicked
    class GizmoSelectionSystem : public Engine::ECS::System {
    public:
      GizmoSelectionSystem() {
        require_components<Components::Transform2D>();
        exclude_components<Components::Gizmo, Components::Text, Components::TileCollider>(); // Only process entities without Gizmo and without Text
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* tr = entity.get_component<Components::Transform2D>();
        if (!tr) return;

        const auto mouse_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

        if (!mouse_opt.has_value()) return;

        const auto mouse = mouse_opt.value();

        // Check if mouse is inside entity bounds
        const float half_w = tr->scale.x * 0.5f;
        const float half_h = tr->scale.y * 0.5f;
        const float left = tr->position.x - half_w;
        const float right = tr->position.x + half_w;
        const float top = tr->position.y - half_h;
        const float bottom = tr->position.y + half_h;

        const bool inside =
          (mouse.x >= left) && (mouse.x <= right) &&
          (mouse.y >= top) && (mouse.y <= bottom);

        // Add Gizmo component when clicked
        if (inside && Engine::Input::is_pressed(KeyCode::MouseLeft) && !TilemapPanel::is_tilemap_mode_enabled()) {
          // Send selection event with start_dragging = true for immediate drag
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(entity.get_id(), true);
        }
      }
    };

    // System to select text entities when clicked
    class GizmoTextSelectionSystem : public Engine::ECS::System {
    public:
      GizmoTextSelectionSystem() {
        require_components<Components::Transform2D, Components::Text, Components::Font>();
        exclude_components<Components::Gizmo>(); // Only process entities without Gizmo
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* tr = entity.get_component<Components::Transform2D>();
        auto* text = entity.get_component<Components::Text>();
        auto* font = entity.get_component<Components::Font>();

        if (!tr || !text || !font) return;

        const auto mouse_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

        if (!mouse_opt.has_value()) return;

        const auto mouse = mouse_opt.value();

        // Accurately measure text dimensions
        // Text scale is calculated as (scale.x + scale.y) * 0.5
        float text_scale = (tr->scale.x + tr->scale.y) * 0.5f;
        Math::Vec2D text_size = Engine::Graphics::measure_text(font->font_handle, text->content, text_scale);

        float text_width = text_size.x;
        float text_height = text_size.y;

        // Use text dimensions for bounding box (top-left aligned)
        const float left = tr->position.x;
        const float right = tr->position.x + text_width;
        const float top = tr->position.y;
        const float bottom = tr->position.y + text_height;

        const bool inside =
          (mouse.x >= left) && (mouse.x <= right) &&
          (mouse.y >= top) && (mouse.y <= bottom);

        // Add Gizmo component when clicked
        if (inside && Engine::Input::is_pressed(KeyCode::MouseLeft) && !TilemapPanel::is_tilemap_mode_enabled()) {
          // Send selection event with start_dragging = true for immediate drag
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(entity.get_id(), true);
        }
      }
    };

    // System to handle dragging and drawing selected entities
    class GizmoManipulationSystem : public Engine::ECS::System {
    public:
      GizmoManipulationSystem() {
        require_components<Components::Transform2D, Components::Gizmo>();
      }

      static inline bool point_in_rect(const Math::Vec2D& p, const Math::Vec2D& c, const Math::Vec2D& size) {
        const float hw = size.x * 0.5f, hh = size.y * 0.5f;
        return (p.x >= c.x - hw) && (p.x <= c.x + hw) && (p.y >= c.y - hh) && (p.y <= c.y + hh);
      }

      static inline float dist_to_segment(const Math::Vec2D& p, const Math::Vec2D& a, const Math::Vec2D& b) {
        const float vx = b.x - a.x, vy = b.y - a.y;
        const float wx = p.x - a.x, wy = p.y - a.y;
        const float vv = vx * vx + vy * vy;
        float t = vv > 0.f ? (wx * vx + wy * vy) / vv : 0.f;
        t = std::max(0.f, std::min(1.f, t));
        const float px = a.x + t * vx, py = a.y + t * vy;
        const float dx = p.x - px, dy = p.y - py;
        return std::sqrt(dx * dx + dy * dy);
      }

      static inline Math::Vec2D rot_deg(const Math::Vec2D& p, const Math::Vec2D& pivot, float deg) {
        const float r = deg * 3.14159265358979323846f / 180.0f;
        const float c = std::cos(r), s = std::sin(r);
        const float x = p.x - pivot.x, y = p.y - pivot.y;
        return { pivot.x + (x * c - y * s), pivot.y + (x * s + y * c) };
      }
      static inline float distance(const Math::Vec2D& a, const Math::Vec2D& b) {
        const float dx = a.x - b.x, dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
      }

      static inline float dot(const Math::Vec2D& a, const Math::Vec2D& b) {
        return a.x * b.x + a.y * b.y;
      }
      static inline Math::Vec2D mul_add(const Math::Vec2D& a, float k, const Math::Vec2D& b) {
        return { a.x + k * b.x, a.y + k * b.y };
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* tr = entity.get_component<Components::Transform2D>();
        auto* gizmo = entity.get_component<Components::Gizmo>();

        if (!tr || !gizmo) return;

        // if tilemap mode is enabled, remove gizmo and return
        if (TilemapPanel::is_tilemap_mode_enabled()) {
          entity.remove_component<Components::Gizmo>();
          return;
        }

        const auto mouse_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

        if (!mouse_opt.has_value()) return;

        const auto mouse = mouse_opt.value();

        // Check if this is a text entity
        auto* text = entity.get_component<Components::Text>();
        auto* font = entity.get_component<Components::Font>();
        bool is_text = (text && font);

        Math::Vec2D rect_center{}, rect_size{};
        Math::Vec2D tl{}, trc{}, br{}, bl{};

        if (is_text) {
          float ts = (tr->scale.x + tr->scale.y) * 0.5f;
          Math::Vec2D sz = Engine::Graphics::measure_text(font->font_handle, text->content, ts);
          rect_center = { tr->position.x + sz.x * 0.5f, tr->position.y + sz.y * 0.5f };
          rect_size = sz;
          tl = { tr->position.x, tr->position.y };
          trc = { tr->position.x + sz.x, tr->position.y };
          br = { tr->position.x + sz.x, tr->position.y + sz.y };
          bl = { tr->position.x, tr->position.y + sz.y };
        }
        else {
          rect_center = tr->position;
          rect_size = tr->scale;
          const float hw = tr->scale.x * 0.5f, hh = tr->scale.y * 0.5f;
          tl = { tr->position.x - hw, tr->position.y - hh };
          trc = { tr->position.x + hw, tr->position.y - hh };
          br = { tr->position.x + hw, tr->position.y + hh };
          bl = { tr->position.x - hw, tr->position.y + hh };
        }

        // Local axes from rotation
        const float ang = tr->rotation * 3.14159265358979323846f / 180.0f;
        const Math::Vec2D xdir{ std::cos(ang), std::sin(ang) };     // local +X
        const Math::Vec2D ydir{ -std::sin(ang), std::cos(ang) };    // local +Y

        // Rotated corners & midpoints
        const float hw = rect_size.x * 0.5f, hh = rect_size.y * 0.5f;
        const Math::Vec2D tl_r = { rect_center.x + (-hw) * xdir.x + (-hh) * ydir.x, rect_center.y + (-hw) * xdir.y + (-hh) * ydir.y };
        const Math::Vec2D tr_r = { rect_center.x + (hw)*xdir.x + (-hh) * ydir.x, rect_center.y + (hw)*xdir.y + (-hh) * ydir.y };
        const Math::Vec2D br_r = { rect_center.x + (hw)*xdir.x + (hh)*ydir.x, rect_center.y + (hw)*xdir.y + (hh)*ydir.y };
        const Math::Vec2D bl_r = { rect_center.x + (-hw) * xdir.x + (hh)*ydir.x, rect_center.y + (-hw) * xdir.y + (hh)*ydir.y };
        const Math::Vec2D mid_top_w{ (tl_r.x + tr_r.x) * 0.5f, (tl_r.y + tr_r.y) * 0.5f };
        const Math::Vec2D mid_right_w{ (tr_r.x + br_r.x) * 0.5f, (tr_r.y + br_r.y) * 0.5f };

        // Rotation ring geometry (ring always centered; knob along -ydir)
        const float ring_radius = 0.5f * std::max(rect_size.x, rect_size.y) + gizmo->rotate_ring_padding;
        const Math::Vec2D ring_center = rect_center;
        const Math::Vec2D knob_center = { ring_center.x - ydir.x * (ring_radius + gizmo->rotate_knob_offset),
                                          ring_center.y - ydir.y * (ring_radius + gizmo->rotate_knob_offset) };

        // Decide which handles are enabled for the current tool mode
        const bool allow_scale = (gizmo->mode == Components::GizmoMode::Scale);
        const bool allow_axes = (gizmo->mode == Components::GizmoMode::ArrowMove);
        const bool allow_body = (gizmo->mode == Components::GizmoMode::GrabMove);
        const bool allow_rot = (gizmo->mode == Components::GizmoMode::Rotate);

        if (Input::is_pressed(KeyCode::MouseLeft)) {
          // inside check via local-space AABB
          Math::Vec2D local = rot_deg(mouse, rect_center, -tr->rotation);
          bool inside = (local.x >= rect_center.x - hw) && (local.x <= rect_center.x + hw) &&
            (local.y >= rect_center.y - hh) && (local.y <= rect_center.y + hh);
        }

        // ---------------- Hover picking ----------------
        gizmo->hover_handle = Components::GizmoHandleType::None;

        auto over_corner = [&](const Math::Vec2D& p) {
          const float r = gizmo->handle_size * 0.6f;
          const float dx = mouse.x - p.x, dy = mouse.y - p.y;
          return (dx * dx + dy * dy) <= r * r;
          };

        if (allow_scale) {
          if (over_corner(tl_r))
            gizmo->hover_handle = Components::GizmoHandleType::ScaleTL;
          else if (over_corner(tr_r))
            gizmo->hover_handle = Components::GizmoHandleType::ScaleTR;
          else if (over_corner(br_r))
            gizmo->hover_handle = Components::GizmoHandleType::ScaleBR;
          else if (over_corner(bl_r))
            gizmo->hover_handle = Components::GizmoHandleType::ScaleBL;
        }
        if (gizmo->hover_handle == Components::GizmoHandleType::None && allow_axes) {
          const float len_x = std::max(12.0f, rect_size.x * gizmo->arrow_length_factor) + gizmo->arrow_outside_extra;
          const float len_y = std::max(12.0f, rect_size.y * gizmo->arrow_length_factor) + gizmo->arrow_outside_extra;
          const float tol = std::max(6.0f, gizmo->arrow_thickness * 2.0f);
          const Math::Vec2D x_a = mid_right_w;
          const Math::Vec2D x_b = { mid_right_w.x + xdir.x * len_x, mid_right_w.y + xdir.y * len_x };
          const Math::Vec2D y_a = mid_top_w;
          const Math::Vec2D y_b = { mid_top_w.x - ydir.x * len_y,  mid_top_w.y - ydir.y * len_y };
          if (dist_to_segment(mouse, x_a, x_b) <= tol) gizmo->hover_handle = Components::GizmoHandleType::AxisX;
          else if (dist_to_segment(mouse, y_a, y_b) <= tol) gizmo->hover_handle = Components::GizmoHandleType::AxisY;
        }
        if (gizmo->hover_handle == Components::GizmoHandleType::None && allow_rot) {
          const float d = distance(mouse, ring_center);
          const float pickTol = gizmo->rotate_ring_thickness * 0.5f + 4.0f;
          if (std::abs(d - ring_radius) <= pickTol ||
            distance(mouse, knob_center) <= gizmo->rotate_handle_radius + 2.0f)
            gizmo->hover_handle = Components::GizmoHandleType::RotateHandle;
        }
        if (gizmo->hover_handle == Components::GizmoHandleType::None && allow_body) {
          Math::Vec2D local = rot_deg(mouse, rect_center, -tr->rotation);
          bool inside = (local.x >= rect_center.x - hw) && (local.x <= rect_center.x + hw) &&
            (local.y >= rect_center.y - hh) && (local.y <= rect_center.y + hh);
          if (inside) gizmo->hover_handle = Components::GizmoHandleType::Body;
        }

        // ---------------- Press ----------------
        if (Engine::Input::is_pressed(KeyCode::MouseLeft)) {
          if (gizmo->hover_handle != Components::GizmoHandleType::None) {
            gizmo->active_handle = gizmo->hover_handle;
            gizmo->start_mouse_world = mouse;
            gizmo->start_pos = tr->position;
            gizmo->start_scale = tr->scale;
            gizmo->start_rotation = tr->rotation;
            gizmo->drag_offset = { mouse.x - tr->position.x, mouse.y - tr->position.y };

            switch (gizmo->active_handle) {
            case Components::GizmoHandleType::ScaleTL:
              gizmo->scale_anchor = br_r;
              break;
            case Components::GizmoHandleType::ScaleTR:
              gizmo->scale_anchor = bl_r;
              break;
            case Components::GizmoHandleType::ScaleBR:
              gizmo->scale_anchor = tl_r;
              break;
            case Components::GizmoHandleType::ScaleBL:
              gizmo->scale_anchor = tr_r;
              break;
            case Components::GizmoHandleType::RotateHandle:
              gizmo->rotate_pivot = rect_center;
              gizmo->start_angle_rad = std::atan2(mouse.y - rect_center.y, mouse.x - rect_center.x);
              break;
            default: break;
            }
            if (is_text) {
              float tscale0 = (tr->scale.x + tr->scale.y) * 0.5f;
              gizmo->text_start_size = Engine::Graphics::measure_text(font->font_handle, text->content, tscale0);
              if (gizmo->text_start_size.x <= 0.f) gizmo->text_start_size.x = 1.f;
              if (gizmo->text_start_size.y <= 0.f) gizmo->text_start_size.y = 1.f;
            }
          }
          else if (!ImGui::GetIO().WantCaptureMouse) {
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
            return;
          }
        }

        // ---------------- Drag ----------------
        const bool held = Engine::Input::is_held(KeyCode::MouseLeft);
        if (!held) { gizmo->is_dragging = false; gizmo->active_handle = Components::GizmoHandleType::None; return; }
        if (gizmo->active_handle == Components::GizmoHandleType::None) return;

        const bool shift = Engine::Input::is_held(KeyCode::ShiftLeft) || Engine::Input::is_held(KeyCode::ShiftRight);

        switch (gizmo->active_handle) {
        case Components::GizmoHandleType::Body:
          if (!allow_body) break; // ignore when mode disallows body
          tr->position.x = mouse.x - gizmo->drag_offset.x;
          tr->position.y = mouse.y - gizmo->drag_offset.y;
          break;

        case Components::GizmoHandleType::AxisX: {
          if (!allow_axes)
            break;
          Math::Vec2D delta{
            mouse.x - gizmo->start_mouse_world.x, mouse.y - gizmo->start_mouse_world.y
          };
          float t = dot(delta, xdir);                      // project on local X
          tr->position = mul_add(gizmo->start_pos, t, xdir);
        } break;

        case Components::GizmoHandleType::AxisY: {
          if (!allow_axes) break;
          Math::Vec2D delta{
            mouse.x - gizmo->start_mouse_world.x, mouse.y - gizmo->start_mouse_world.y
          };
          float t = dot(delta, ydir);                      // project on local Y
          tr->position = mul_add(gizmo->start_pos, t, ydir);
        } break;

        case Components::GizmoHandleType::RotateHandle: {
          if (!allow_rot) break;
          float ang_now = std::atan2(mouse.y - gizmo->rotate_pivot.y, mouse.x - gizmo->rotate_pivot.x);
          float delta_deg = (ang_now - gizmo->start_angle_rad) * (180.0f / 3.14159265359f);
          if (shift) {
            const float snap = 15.0f;
            delta_deg = std::round(delta_deg / snap) * snap;
          }
          tr->rotation = gizmo->start_rotation + delta_deg;
        } break;

        case Components::GizmoHandleType::ScaleTL:
        case Components::GizmoHandleType::ScaleTR:
        case Components::GizmoHandleType::ScaleBR:
        case Components::GizmoHandleType::ScaleBL:
        {
          if (!allow_scale) break;
          // local-space scale so the dragged corner follows mouse
          const Math::Vec2D anchor_local = rot_deg(gizmo->scale_anchor, rect_center, -tr->rotation);
          const Math::Vec2D mouse_local = rot_deg(mouse, rect_center, -tr->rotation);
          Math::Vec2D delta_local{ mouse_local.x - anchor_local.x, mouse_local.y - anchor_local.y };
          Math::Vec2D center_local{ anchor_local.x + delta_local.x * 0.5f,
                                    anchor_local.y + delta_local.y * 0.5f };
          Math::Vec2D size_local{ std::max(4.0f, std::abs(delta_local.x)),
                                  std::max(4.0f, std::abs(delta_local.y)) };
          if (shift) {
            float aspect = (gizmo->start_scale.y != 0.f) ? (gizmo->start_scale.x / gizmo->start_scale.y) : 1.f;
            if (size_local.y > 0.f) size_local.x = std::max(4.0f, std::abs(size_local.y * aspect));
          }
          const Math::Vec2D center_world = rot_deg(center_local, rect_center, tr->rotation);
          tr->position = center_world;
          tr->scale = { size_local.x, size_local.y };

          // Text: keep top-left anchored scaling semantics
          if (is_text) {
            const float minx = std::min(gizmo->scale_anchor.x, mouse.x);
            const float miny = std::min(gizmo->scale_anchor.y, mouse.y);
            const float maxx = std::max(gizmo->scale_anchor.x, mouse.x);
            const float maxy = std::max(gizmo->scale_anchor.y, mouse.y);
            Math::Vec2D desired{ std::max(4.0f, maxx - minx), std::max(4.0f, maxy - miny) };
            float sx = desired.x / gizmo->text_start_size.x;
            float sy = desired.y / gizmo->text_start_size.y;
            if (shift) {
              float s = std::max(sx, sy); sx = sy = s;
            }
            tr->scale.x = gizmo->start_scale.x * sx;
            tr->scale.y = gizmo->start_scale.y * sy;
            tr->position = { minx, miny };
          }
        } break;

        default: break;

        }
      }

      void draw_entity(const Engine::ECS::Entity& entity) override {
        auto* tr = entity.get_component<Components::Transform2D>();
        auto* gizmo = entity.get_component<Components::Gizmo>();

        if (!tr || !gizmo) return;

        // Check if this is a text entity
        auto* text = entity.get_component<Components::Text>();
        auto* font = entity.get_component<Components::Font>();
        bool is_text = (text && font);

        Math::Vec2D rect_center{}, rect_size{};
        if (is_text) {
          float ts = (tr->scale.x + tr->scale.y) * 0.5f;
          Math::Vec2D sz = Engine::Graphics::measure_text(font->font_handle, text->content, ts);
          rect_center = { tr->position.x + sz.x * 0.5f, tr->position.y + sz.y * 0.5f };
          rect_size = sz;
        }
        else {
          rect_center = tr->position;
          rect_size = tr->scale;
        }

        Engine::Graphics::draw_rect_outline(
          rect_center,
          rect_size,
          tr->rotation,
          gizmo->outline_thickness,
          gizmo->outline_color
        );

        // Rotated geometry for handles/arrows
        const float ang = tr->rotation * 3.14159265358979323846f / 180.0f;
        const Math::Vec2D xdir{ std::cos(ang), std::sin(ang) };
        const Math::Vec2D ydir{ -std::sin(ang), std::cos(ang) };
        const float hw = rect_size.x * 0.5f, hh = rect_size.y * 0.5f;
        const Math::Vec2D tl_r = { rect_center.x + (-hw) * xdir.x + (-hh) * ydir.x, rect_center.y + (-hw) * xdir.y + (-hh) * ydir.y };
        const Math::Vec2D tr_r = { rect_center.x + (hw)*xdir.x + (-hh) * ydir.x, rect_center.y + (hw)*xdir.y + (-hh) * ydir.y };
        const Math::Vec2D br_r = { rect_center.x + (hw)*xdir.x + (hh)*ydir.x, rect_center.y + (hw)*xdir.y + (hh)*ydir.y };
        const Math::Vec2D bl_r = { rect_center.x + (-hw) * xdir.x + (hh)*ydir.x, rect_center.y + (-hw) * xdir.y + (hh)*ydir.y };
        const Math::Vec2D mid_top_w{ (tl_r.x + tr_r.x) * 0.5f, (tl_r.y + tr_r.y) * 0.5f };
        const Math::Vec2D mid_right_w{ (tr_r.x + br_r.x) * 0.5f, (tr_r.y + br_r.y) * 0.5f };

        // Scale handles
        if (gizmo->mode == Components::GizmoMode::Scale) {
          auto draw_handle = [&](const Math::Vec2D& c, bool hovered) {
            const Math::Vec4D col = hovered ? gizmo->handle_hover_color : gizmo->handle_color;
            Graphics::draw_rect_outline(c, { gizmo->handle_size, gizmo->handle_size }, tr->rotation, 2.0f, col);
            };
          draw_handle(tl_r, gizmo->hover_handle == Components::GizmoHandleType::ScaleTL);
          draw_handle(tr_r, gizmo->hover_handle == Components::GizmoHandleType::ScaleTR);
          draw_handle(br_r, gizmo->hover_handle == Components::GizmoHandleType::ScaleBR);
          draw_handle(bl_r, gizmo->hover_handle == Components::GizmoHandleType::ScaleBL);
        }

        // Rotation ring + crosshair + knob
        if (gizmo->mode == Components::GizmoMode::Rotate) {
          const float ring_radius = 0.5f * std::max(rect_size.x, rect_size.y) + gizmo->rotate_ring_padding;
          const Math::Vec2D center = rect_center;
          const Math::Vec2D knob = { center.x - ydir.x * (ring_radius + gizmo->rotate_knob_offset),
                                       center.y - ydir.y * (ring_radius + gizmo->rotate_knob_offset) };

          const Math::Vec4D ring_col = (gizmo->hover_handle == Components::GizmoHandleType::RotateHandle)
            ? gizmo->handle_hover_color : gizmo->handle_color;

          Graphics::draw_circle_outline(center, ring_radius, gizmo->rotate_ring_thickness, ring_col, 64);

          // crosshair aligned to local axes
          const Math::Vec4D colX{ 0.2f, 1.0f, 0.2f, 1.0f };
          const Math::Vec4D colY{ 1.0f, 0.2f, 0.2f, 1.0f };
          Graphics::draw_line({ center.x - xdir.x * ring_radius, center.y - xdir.y * ring_radius },
            { center.x + xdir.x * ring_radius, center.y + xdir.y * ring_radius },
            1.5f, colX);
          Graphics::draw_line({ center.x - ydir.x * ring_radius, center.y - ydir.y * ring_radius },
            { center.x + ydir.x * ring_radius, center.y + ydir.y * ring_radius },
            1.5f, colY);

          // knob outline
          Graphics::draw_circle_outline(knob, gizmo->rotate_handle_radius, gizmo->outline_thickness, ring_col, 24);
        }

        // Axis arrows along local axes
        if (gizmo->mode == Components::GizmoMode::ArrowMove) {
          const float len_x = std::max(12.0f, rect_size.x * gizmo->arrow_length_factor) + gizmo->arrow_outside_extra;
          const float len_y = std::max(12.0f, rect_size.y * gizmo->arrow_length_factor) + gizmo->arrow_outside_extra;

          // X axis
          {
            Math::Vec2D from = mid_right_w;
            Math::Vec2D shaft_center{ from.x + xdir.x * (len_x * 0.5f), from.y + xdir.y * (len_x * 0.5f) };
            Math::Vec4D col = (gizmo->hover_handle == Components::GizmoHandleType::AxisX) ? gizmo->handle_hover_color : gizmo->handle_color;
            Graphics::draw_rect_outline(shaft_center, { len_x, gizmo->arrow_thickness }, tr->rotation, gizmo->arrow_thickness, col);
            const float hs = gizmo->arrow_head_size;
            const Math::Vec2D tip{ from.x + xdir.x * len_x, from.y + xdir.y * len_x };
            Graphics::draw_rect_outline(tip, { hs, gizmo->arrow_thickness }, tr->rotation + 45.0f, gizmo->arrow_thickness, col);
            Graphics::draw_rect_outline(tip, { hs, gizmo->arrow_thickness }, tr->rotation - 45.0f, gizmo->arrow_thickness, col);
          }

          // Y axis
          {
            Math::Vec2D from = mid_top_w;
            Math::Vec2D shaft_center{ from.x - ydir.x * (len_y * 0.5f), from.y - ydir.y * (len_y * 0.5f) };
            Math::Vec4D col = (gizmo->hover_handle == Components::GizmoHandleType::AxisY) ? gizmo->handle_hover_color : gizmo->handle_color;
            Graphics::draw_rect_outline(shaft_center, { len_y, gizmo->arrow_thickness }, tr->rotation + 90.0f, gizmo->arrow_thickness, col);
            const float hs = gizmo->arrow_head_size;
            const Math::Vec2D tip{ from.x - ydir.x * len_y, from.y - ydir.y * len_y };
            Graphics::draw_rect_outline(tip, { hs, gizmo->arrow_thickness }, tr->rotation + 90.0f + 45.0f, gizmo->arrow_thickness, col);
            Graphics::draw_rect_outline(tip, { hs, gizmo->arrow_thickness }, tr->rotation + 90.0f - 45.0f, gizmo->arrow_thickness, col);
          }
        }

        // ---------- ImGui Toolbar ----------
        {
          Components::Gizmo* gizmo_nc = const_cast<Components::Gizmo*>(gizmo);


          ImGui::SetNextWindowBgAlpha(0.9f);
          ImGui::SetNextWindowSize(ImVec2(160, 200), ImGuiCond_Once);

          if (ImGui::Begin("Gizmo Tools", nullptr,
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize))
          {
            int mode = static_cast<int>(gizmo_nc->mode);

            ImGui::TextUnformatted("Select Tool");
            ImGui::Separator();

            if (ImGui::RadioButton("Grab / Move", mode == 0)) mode = 0;
            if (ImGui::RadioButton("Arrows (X/Y)", mode == 1)) mode = 1;

            //Disable rotation for text
            if (is_text) ImGui::BeginDisabled();
            if (ImGui::RadioButton("Rotate", mode == 2)) mode = 2;
            if (is_text) ImGui::EndDisabled();

            if (ImGui::RadioButton("Scale", mode == 3)) mode = 3;

            ImGui::Separator();

            gizmo_nc->mode = static_cast<Components::GizmoMode>(mode);
          }
          ImGui::End();

        }

      }
    };

  } // namespace Systems
} // namespace Engine
