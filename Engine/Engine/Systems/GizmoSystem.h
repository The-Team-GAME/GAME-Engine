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
        exclude_components<Components::Gizmo, Components::Text>(); // Only process entities without Gizmo and without Text
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* tr = entity.get_component<Components::Transform2D>();
        if (!tr) return;

        // Don't select entities when Ctrl is held (camera control mode)
        bool ctrl_held = Engine::Input::is_held(Engine::KeyCode::CtrlLeft) ||
          Engine::Input::is_held(Engine::KeyCode::CtrlRight);

        if (ctrl_held) return;

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
        if (inside && Engine::Input::is_pressed(KeyCode::MouseLeft)) {
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

        // Don't select entities when Ctrl is held (camera control mode)
        bool ctrl_held = Engine::Input::is_held(Engine::KeyCode::CtrlLeft) ||
          Engine::Input::is_held(Engine::KeyCode::CtrlRight);

        if (ctrl_held) return;

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
        if (inside && Engine::Input::is_pressed(KeyCode::MouseLeft)) {
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

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* tr = entity.get_component<Components::Transform2D>();
        auto* gizmo = entity.get_component<Components::Gizmo>();

        if (!tr || !gizmo) return;

        // Check if Ctrl is held (camera control mode)
        bool ctrl_held = Engine::Input::is_held(Engine::KeyCode::CtrlLeft) ||
          Engine::Input::is_held(Engine::KeyCode::CtrlRight);

        // If Ctrl is held, stop any ongoing drag but keep the gizmo visible
        if (ctrl_held) {
          gizmo->is_dragging = false;
          gizmo->active_handle = Components::GizmoHandleType::None;
          return;
        }

        const auto mouse_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

        if (!mouse_opt.has_value()) return;

        const auto mouse = mouse_opt.value();

        // Check if this is a text entity
        auto* text = entity.get_component<Components::Text>();
        auto* font = entity.get_component<Components::Font>();
        bool is_text = (text && font);
        //bool inside = false;

        Math::Vec2D rect_center{}, rect_size{};
        Math::Vec2D tl{}, trc{}, br{}, bl{};

        /*
        if (text && font) {
          // Calculate bounds for text entity (top-left aligned)
          float text_scale = (tr->scale.x + tr->scale.y) * 0.5f;
          Math::Vec2D text_size = Engine::Graphics::measure_text(font->font_handle, text->content, text_scale);

          const float left = tr->position.x;
          const float right = tr->position.x + text_size.x;
          const float top = tr->position.y;
          const float bottom = tr->position.y + text_size.y;

          inside = (mouse.x >= left) && (mouse.x <= right) &&
            (mouse.y >= top) && (mouse.y <= bottom);
        }
        else {
          // Calculate bounds for sprite entity (centered)
          const float half_w = tr->scale.x * 0.5f;
          const float half_h = tr->scale.y * 0.5f;
          const float left = tr->position.x - half_w;
          const float right = tr->position.x + half_w;
          const float top = tr->position.y - half_h;
          const float bottom = tr->position.y + half_h;

          inside = (mouse.x >= left) && (mouse.x <= right) &&
            (mouse.y >= top) && (mouse.y <= bottom);
        }

        // Handle starting drag on click
        if (Engine::Input::is_pressed(KeyCode::MouseLeft)) {
          if (inside) {
            // Start dragging - calculate offset from entity center to mouse position
            gizmo->drag_offset = Math::Vec2D(mouse.x - tr->position.x, mouse.y - tr->position.y);
            gizmo->is_dragging = true;
          }
          else {
            // Only deselect if click was NOT on an ImGui window
            if (!ImGui::GetIO().WantCaptureMouse) {
              // Clicked outside - deselect (send deselection event)
              Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
              return;
            }
          }
        }

        // Handle dragging
        if (gizmo->is_dragging) {
          if (Engine::Input::is_held(KeyCode::MouseLeft)) {
            // Update position while dragging (accounting for offset)
            tr->position.x = mouse.x - gizmo->drag_offset.x;
            tr->position.y = mouse.y - gizmo->drag_offset.y;
          }
          else {
            // Stop dragging when mouse is released
            gizmo->is_dragging = false;
          }
        }
        */

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
        
        Math::Vec2D mid_top{ (tl.x + trc.x) * 0.5f, (tl.y + trc.y) * 0.5f };
        Math::Vec2D mid_right{ (trc.x + br.x) * 0.5f, (trc.y + br.y) * 0.5f };
        
        
        // Decide which handles are enabled for the current tool mode
        const bool allow_scale = (gizmo->mode == Components::GizmoMode::Scale);
        const bool allow_axes = (gizmo->mode == Components::GizmoMode::ArrowMove);
        const bool allow_body = (gizmo->mode == Components::GizmoMode::GrabMove);
        const bool allow_rot = (gizmo->mode == Components::GizmoMode::Rotate) && !is_text;

        // ---------------- Hover picking ----------------
        gizmo->hover_handle = Components::GizmoHandleType::None;

        const Math::Vec2D hsize{ gizmo->handle_size, gizmo->handle_size };

        if (allow_scale) {
          if (point_in_rect(mouse, tl, hsize)) gizmo->hover_handle = Components::GizmoHandleType::ScaleTL;
          else if (point_in_rect(mouse, trc, hsize)) gizmo->hover_handle = Components::GizmoHandleType::ScaleTR;
          else if (point_in_rect(mouse, br, hsize)) gizmo->hover_handle = Components::GizmoHandleType::ScaleBR;
          else if (point_in_rect(mouse, bl, hsize)) gizmo->hover_handle = Components::GizmoHandleType::ScaleBL;
        }
        if (gizmo->hover_handle == Components::GizmoHandleType::None && allow_axes) {
          const float tol = std::max(6.0f, gizmo->arrow_thickness * 2.0f);
          const bool over_x = dist_to_segment(mouse, mid_right, { mid_right.x + (std::max(12.0f, rect_size.x * gizmo->arrow_length_factor) + gizmo->arrow_outside_extra), mid_right.y }) <= tol;
          const bool over_y = dist_to_segment(mouse, mid_top, { mid_top.x,   mid_top.y - (std::max(12.0f, rect_size.y * gizmo->arrow_length_factor) + gizmo->arrow_outside_extra) }) <= tol;
          if (over_x) gizmo->hover_handle = Components::GizmoHandleType::AxisX;
          else if (over_y) gizmo->hover_handle = Components::GizmoHandleType::AxisY;
        }
        if (gizmo->hover_handle == Components::GizmoHandleType::None && allow_rot) {
          // set hover to RotateHandle if you draw it
        }
        if (gizmo->hover_handle == Components::GizmoHandleType::None && allow_body) {
          if (point_in_rect(mouse, rect_center, rect_size))
            gizmo->hover_handle = Components::GizmoHandleType::Body;
        }

        // Open toolbar only when interacting with an enabled handle/body
        if (Engine::Input::is_pressed(KeyCode::MouseLeft) && gizmo->hover_handle != Components::GizmoHandleType::None) {
          gizmo->toolbar_open = true;
        }

        // ---------------- Press ----------------
        if (Engine::Input::is_pressed(KeyCode::MouseLeft)) {
          if (gizmo->hover_handle != Components::GizmoHandleType::None) {
            gizmo->active_handle = gizmo->hover_handle;
            gizmo->start_mouse_world = mouse;
            gizmo->start_pos = tr->position;
            gizmo->start_scale = tr->scale;
            gizmo->drag_offset = { mouse.x - tr->position.x, mouse.y - tr->position.y };
            switch (gizmo->active_handle) {
            case Components::GizmoHandleType::ScaleTL: gizmo->scale_anchor = br;  break;
            case Components::GizmoHandleType::ScaleTR: gizmo->scale_anchor = bl;  break;
            case Components::GizmoHandleType::ScaleBR: gizmo->scale_anchor = tl;  break;
            case Components::GizmoHandleType::ScaleBL: gizmo->scale_anchor = trc; break;
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

        case Components::GizmoHandleType::AxisX:
          if (!allow_axes) break;
          {
            float target_x = mouse.x - gizmo->drag_offset.x;
            tr->position.x = gizmo->start_pos.x + (target_x - gizmo->start_pos.x) * gizmo->axis_move_slow_factor;
            tr->position.y = gizmo->start_pos.y;
          }
          break;

        case Components::GizmoHandleType::AxisY:
          if (!allow_axes) break;
          {
            float target_y = mouse.y - gizmo->drag_offset.y;
            tr->position.x = gizmo->start_pos.x;
            tr->position.y = gizmo->start_pos.y + (target_y - gizmo->start_pos.y) * gizmo->axis_move_slow_factor;
          }
          break;

        case Components::GizmoHandleType::ScaleTL:
        case Components::GizmoHandleType::ScaleTR:
        case Components::GizmoHandleType::ScaleBR:
        case Components::GizmoHandleType::ScaleBL:
          if (!allow_scale) break;
          if (!is_text) {
            Math::Vec2D half{ (mouse.x - gizmo->scale_anchor.x) * 0.5f,
                              (mouse.y - gizmo->scale_anchor.y) * 0.5f };
            Math::Vec2D new_center{ gizmo->scale_anchor.x + half.x, gizmo->scale_anchor.y + half.y };
            Math::Vec2D new_size{ std::max(4.0f, std::abs(half.x) * 2.0f),
                                  std::max(4.0f, std::abs(half.y) * 2.0f) };
            if (shift) {
              float aspect = (gizmo->start_scale.y != 0.f) ? (gizmo->start_scale.x / gizmo->start_scale.y) : 1.f;
              if (new_size.y > 0.f) new_size.x = std::max(4.0f, std::abs(new_size.y * aspect));
            }
            tr->position = new_center;
            tr->scale = new_size;
          }
          else {
            const float minx = std::min(gizmo->scale_anchor.x, mouse.x);
            const float miny = std::min(gizmo->scale_anchor.y, mouse.y);
            const float maxx = std::max(gizmo->scale_anchor.x, mouse.x);
            const float maxy = std::max(gizmo->scale_anchor.y, mouse.y);
            Math::Vec2D desired{ std::max(4.0f, maxx - minx), std::max(4.0f, maxy - miny) };
            float sx = desired.x / gizmo->text_start_size.x;
            float sy = desired.y / gizmo->text_start_size.y;
            if (shift) { float s = std::max(sx, sy); sx = sy = s; }
            tr->scale.x = gizmo->start_scale.x * sx;
            tr->scale.y = gizmo->start_scale.y * sy;
            tr->position = { minx, miny };
          }
          break;

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

        /*
        if (text && font) {
          // Draw outline for text entity using accurate text bounds
          float text_scale = (tr->scale.x + tr->scale.y) * 0.5f;
          Math::Vec2D text_size = Engine::Graphics::measure_text(font->font_handle, text->content, text_scale);

          float text_width = text_size.x;
          float text_height = text_size.y;

          // Draw outline at text position (top-left aligned)
          Math::Vec2D outline_center(tr->position.x + text_width * 0.5f, tr->position.y + text_height * 0.5f);
          Math::Vec2D outline_size(text_width, text_height);

          Engine::Graphics::draw_rect_outline(
            outline_center,
            outline_size,
            0.0f, // Text doesn't rotate
            gizmo->outline_thickness,
            gizmo->outline_color
          );
        }
        else {
          // Draw outline for regular sprite entity
          Engine::Graphics::draw_rect_outline(
            tr->position,
            tr->scale,
            tr->rotation,
            gizmo->outline_thickness,
            gizmo->outline_color
          );
        }
        */

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
          rect_center, rect_size, is_text ? 0.0f : tr->rotation,
          gizmo->outline_thickness, gizmo->outline_color);

        const float hw = rect_size.x * 0.5f, hh = rect_size.y * 0.5f;
        Math::Vec2D tl{ rect_center.x - hw, rect_center.y - hh };
        Math::Vec2D trc{ rect_center.x + hw, rect_center.y - hh };
        Math::Vec2D br{ rect_center.x + hw, rect_center.y + hh };
        Math::Vec2D bl{ rect_center.x - hw, rect_center.y + hh };
        if (is_text) {
          float ts = (tr->scale.x + tr->scale.y) * 0.5f;
          Math::Vec2D sz = Engine::Graphics::measure_text(font->font_handle, text->content, ts);
          tl = { tr->position.x,          tr->position.y };
          trc = { tr->position.x + sz.x,   tr->position.y };
          br = { tr->position.x + sz.x,   tr->position.y + sz.y };
          bl = { tr->position.x,          tr->position.y + sz.y };
        }

        if (gizmo->mode == Components::GizmoMode::Scale) {
          auto draw_handle = [&](const Math::Vec2D& c, bool hovered) {
            const Math::Vec4D col = hovered ? gizmo->handle_hover_color : gizmo->handle_color;
            Graphics::draw_rect_outline(c, { gizmo->handle_size, gizmo->handle_size }, 0.0f, 2.0f, col);
            };
          draw_handle(tl, gizmo->hover_handle == Components::GizmoHandleType::ScaleTL);
          draw_handle(trc, gizmo->hover_handle == Components::GizmoHandleType::ScaleTR);
          draw_handle(br, gizmo->hover_handle == Components::GizmoHandleType::ScaleBR);
          draw_handle(bl, gizmo->hover_handle == Components::GizmoHandleType::ScaleBL);
        }

        // arrows
        if (gizmo->mode == Components::GizmoMode::ArrowMove) {
          const Math::Vec2D mid_top{ (tl.x + trc.x) * 0.5f, (tl.y + trc.y) * 0.5f };
          const Math::Vec2D mid_right{ (trc.x + br.x) * 0.5f, (trc.y + br.y) * 0.5f };
          const float len_x = std::max(12.0f, rect_size.x * gizmo->arrow_length_factor) + gizmo->arrow_outside_extra;
          const float len_y = std::max(12.0f, rect_size.y * gizmo->arrow_length_factor) + gizmo->arrow_outside_extra;

          // X axis
          {
            Math::Vec2D from = mid_right;
            Math::Vec2D to{ mid_right.x + len_x, mid_right.y };
            Math::Vec2D shaft_center{ (from.x + to.x) * 0.5f, from.y };
            Math::Vec4D col = (gizmo->hover_handle == Components::GizmoHandleType::AxisX) ? gizmo->handle_hover_color : gizmo->handle_color;

            // shaft
            Graphics::draw_rect_outline(shaft_center, { len_x, gizmo->arrow_thickness }, 0.0f, gizmo->arrow_thickness, col);
            // head
            const float hs = gizmo->arrow_head_size;
            const Math::Vec2D head_center = to;
            Graphics::draw_rect_outline({ head_center.x - hs * 0.35f, head_center.y }, { hs, gizmo->arrow_thickness }, 45.0f, gizmo->arrow_thickness, col);
            Graphics::draw_rect_outline({ head_center.x - hs * 0.35f, head_center.y }, { hs, gizmo->arrow_thickness }, -45.0f, gizmo->arrow_thickness, col);
          }

          // Y axis
          {
            Math::Vec2D from = mid_top;
            Math::Vec2D to{ mid_top.x, mid_top.y - len_y };
            Math::Vec2D shaft_center{ from.x, (from.y + to.y) * 0.5f };
            Math::Vec4D col = (gizmo->hover_handle == Components::GizmoHandleType::AxisY) ? gizmo->handle_hover_color : gizmo->handle_color;

            // shaft
            Graphics::draw_rect_outline(shaft_center, { gizmo->arrow_thickness, len_y }, 0.0f, gizmo->arrow_thickness, col);
            // head
            const float hs = gizmo->arrow_head_size;
            const Math::Vec2D head_center = to;
            Graphics::draw_rect_outline({ head_center.x, head_center.y + hs * 0.35f }, { hs, gizmo->arrow_thickness }, 45.0f, gizmo->arrow_thickness, col);
            Graphics::draw_rect_outline({ head_center.x, head_center.y + hs * 0.35f }, { hs, gizmo->arrow_thickness }, -45.0f, gizmo->arrow_thickness, col);
          }
        }

        /*
        if (gizmo->mode == Components::GizmoMode::Rotate && !is_text) {
          const Math::Vec2D mid_top{ (tl.x + trc.x) * 0.5f, (tl.y + trc.y) * 0.5f };
          Math::Vec2D center{ mid_top.x, mid_top.y - gizmo->rotate_handle_offset };
          // Small circle outline via many short segments (approx by rectangles)
          Graphics::draw_circle_outline(center, gizmo->rotate_handle_radius,
            (gizmo->hover_handle == Components::GizmoHandleType::RotateHandle) ? gizmo->handle_hover_color : gizmo->handle_color);
        }
        */

        // ---------- ImGui Toolbar ----------
        {
          Components::Gizmo* gizmo_nc = const_cast<Components::Gizmo*>(gizmo);

          if (gizmo_nc->toolbar_open) {
            bool open = gizmo_nc->toolbar_open; // local mutable copy for ImGui

            ImGui::SetNextWindowBgAlpha(0.9f);
            ImGui::SetNextWindowSize(ImVec2(160, 200), ImGuiCond_Once);

            if (ImGui::Begin("Gizmo Tools", &open,
              ImGuiWindowFlags_NoSavedSettings |
              ImGuiWindowFlags_AlwaysAutoResize))
            {
              int mode = static_cast<int>(gizmo_nc->mode);

              ImGui::TextUnformatted("Select Tool");
              ImGui::Separator();

              if (ImGui::RadioButton("Grab / Move", mode == 0)) mode = 0;
              if (ImGui::RadioButton("Arrows (X/Y)", mode == 1)) mode = 1;

              // Disable rotate for text
              if (is_text) ImGui::BeginDisabled();
              if (ImGui::RadioButton("Rotate", mode == 2)) mode = 2;
              if (is_text) ImGui::EndDisabled();

              if (ImGui::RadioButton("Scale", mode == 3)) mode = 3;

              ImGui::Separator();
              ImGui::SliderFloat("Axis Slow", &gizmo_nc->axis_move_slow_factor, 0.05f, 1.0f, "%.2f");

              gizmo_nc->mode = static_cast<Components::GizmoMode>(mode);
            }
            ImGui::End();

            // reflect close button / toggles
            gizmo_nc->toolbar_open = open;
          }
        }

      }
  };

  } // namespace Systems
} // namespace Engine
