//-----------------------------------------------------------------------------------
// file:    GizmoSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (40%)
//          Primary author
// 
//          Ong Zi Ming (60%)
//          Secondary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of the gizmo system for entity selection and manipulation
//          in the game editor. Handles mouse picking for sprites and text, visual
//          feedback (outlines, handles, arrows, rotation ring), and drag
//          behaviour for moving, rotating and scaling entities. Integrates
//          with the input, rendering and event systems for undo/redo support.
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
#include "../GUI/ViewportPanel.h"
#include "../ImGuiManager.h"
#include "Precompiled.h"

namespace Engine
{
  namespace Systems
  {
    // System to add Gizmo component when entities with Transform2D are clicked
    class GizmoSelectionSystem : public Engine::ECS::System {
    public:
      GizmoSelectionSystem() {
        require_components<Components::Transform2D>();
        exclude_components<Components::Text, Components::TileCollider>(); // Only process entities without Text
        set_editor_system(); // Editor system - runs even when paused
      }

      void update() override {
        // Ctrl+Shift+A: deselect all entities
        if (Engine::Input::is_pressed(KeyCode::A) &&
          (Engine::Input::is_held(KeyCode::CtrlLeft) || Engine::Input::is_held(KeyCode::CtrlRight)) &&
          (Engine::Input::is_held(KeyCode::ShiftLeft) || Engine::Input::is_held(KeyCode::ShiftRight))) {
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
          return;
        }

        // If a currently selected entity (with Gizmo) is under the mouse, don't change selection
        {
          bool selected_under_mouse = false;
          Engine::ECS::for_each_entity_with<Components::Gizmo>([&](Engine::ECS::Entity gizmo_entity) {
            auto* tr = gizmo_entity.get_component<Components::Transform2D>();
            if (!tr) return;
            // UI entities use screen-space coordinates (not affected by camera)
            const auto mouse_opt = gizmo_entity.has_component<Components::UI>()
              ? Engine::Input::get_mouse_position_if_in_viewport()
              : Engine::Input::get_mouse_world_position_if_in_viewport();
            if (!mouse_opt.has_value()) return;
            const auto mouse = mouse_opt.value();
            const float hw = tr->scale.x * 0.5f, hh = tr->scale.y * 0.5f;
            if (mouse.x >= tr->position.x - hw && mouse.x <= tr->position.x + hw &&
              mouse.y >= tr->position.y - hh && mouse.y <= tr->position.y + hh) {
              selected_under_mouse = true;
            }
            });
          if (selected_under_mouse) return;
        }

        Engine::ECS::EntityID best_entity = Engine::ECS::INVALID_ENTITY;

        auto& containers = Engine::ECS::Internal::get_containers();
        for (Engine::ECS::LayerID layer = 0; layer < containers.layer_entities.size(); ++layer) {
          if (!get_layer_signature().test(layer))
            continue;
          if (!containers.layer_active_state.test(layer))
            continue;

          for (Engine::ECS::EntityID entityId : containers.layer_entities[layer]) {
            if (!get_entities().contains(entityId))
              continue;

            Engine::ECS::Entity entity(entityId);
            if (is_entity_under_mouse(entity)) {
              best_entity = entityId;
            }
          }
        }

        if (Engine::Input::is_pressed(KeyCode::MouseLeft) && !TilemapPanel::is_tilemap_mode_enabled()) {
          if (best_entity != Engine::ECS::INVALID_ENTITY) {
            Engine::ECS::Entity entity(best_entity);
            // Only select if not already selected (has Gizmo)
            if (!entity.has_component<Components::Gizmo>()) {
              Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(best_entity, true);
            }
          }
          else if (!ImGui::GetIO().WantCaptureMouse) {
            // Clicked on empty space - deselect all
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
          }
        }
      }

      bool is_entity_under_mouse(Engine::ECS::Entity entity) {
        auto* tr = entity.get_component<Components::Transform2D>();
        if (!tr) return false;

        // UI entities use screen-space coordinates (not affected by camera)
        const auto mouse_opt = entity.has_component<Components::UI>()
          ? Engine::Input::get_mouse_position_if_in_viewport()
          : Engine::Input::get_mouse_world_position_if_in_viewport();
        if (!mouse_opt.has_value()) return false;
        const auto mouse = mouse_opt.value();

        // Check if mouse is inside entity bounds
        const float half_w = tr->scale.x * 0.5f;
        const float half_h = tr->scale.y * 0.5f;
        const float left = tr->position.x - half_w;
        const float right = tr->position.x + half_w;
        const float top = tr->position.y - half_h;
        const float bottom = tr->position.y + half_h;

        return (mouse.x >= left) && (mouse.x <= right) &&
          (mouse.y >= top) && (mouse.y <= bottom);
      }

      //void update_entity(Engine::ECS::Entity entity) override {}
    };

    // System to select text entities when clicked
    class GizmoTextSelectionSystem : public Engine::ECS::System {
    public:
      GizmoTextSelectionSystem() {
        require_components<Components::Transform2D, Components::Text, Components::Font>();
        set_editor_system(); // Editor system - runs even when paused
      }

      void update() override {
        // If a currently selected entity (with Gizmo) is under the mouse, don't change selection
        {
          bool selected_under_mouse = false;
          Engine::ECS::for_each_entity_with<Components::Gizmo>([&](Engine::ECS::Entity gizmo_entity) {
            auto* tr = gizmo_entity.get_component<Components::Transform2D>();
            if (!tr) return;
            // UI entities use screen-space coordinates (not affected by camera)
            const auto mouse_opt = gizmo_entity.has_component<Components::UI>()
              ? Engine::Input::get_mouse_position_if_in_viewport()
              : Engine::Input::get_mouse_world_position_if_in_viewport();
            if (!mouse_opt.has_value()) return;
            const auto mouse = mouse_opt.value();
            const float hw = tr->scale.x * 0.5f, hh = tr->scale.y * 0.5f;
            if (mouse.x >= tr->position.x - hw && mouse.x <= tr->position.x + hw &&
              mouse.y >= tr->position.y - hh && mouse.y <= tr->position.y + hh) {
              selected_under_mouse = true;
            }
            });
          if (selected_under_mouse) return;
        }

        Engine::ECS::EntityID best_entity = Engine::ECS::INVALID_ENTITY;

        auto& containers = Engine::ECS::Internal::get_containers();
        for (Engine::ECS::LayerID layer = 0; layer < containers.layer_entities.size(); ++layer) {
          if (!get_layer_signature().test(layer))
            continue;
          if (!containers.layer_active_state.test(layer))
            continue;

          for (Engine::ECS::EntityID entityId : containers.layer_entities[layer]) {
            if (!get_entities().contains(entityId))
              continue;

            Engine::ECS::Entity entity(entityId);
            if (is_entity_under_mouse(entity)) {
              best_entity = entityId;
            }
          }
        }

        if (Engine::Input::is_pressed(KeyCode::MouseLeft) && !TilemapPanel::is_tilemap_mode_enabled()) {
          if (best_entity != Engine::ECS::INVALID_ENTITY) {
            Engine::ECS::Entity entity(best_entity);
            if (!entity.has_component<Components::Gizmo>()) {
              Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(best_entity, true);
            }
          }
          else if (!ImGui::GetIO().WantCaptureMouse) {
            // Clicked on empty space - deselect all
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
          }
        }
      }

      bool is_entity_under_mouse(Engine::ECS::Entity entity) {
        auto* tr = entity.get_component<Components::Transform2D>();
        auto* text = entity.get_component<Components::Text>();
        auto* font = entity.get_component<Components::Font>();

        if (!tr || !text || !font) return false;

        // UI entities use screen-space coordinates (not affected by camera)
        const auto mouse_opt = entity.has_component<Components::UI>()
          ? Engine::Input::get_mouse_position_if_in_viewport()
          : Engine::Input::get_mouse_world_position_if_in_viewport();
        if (!mouse_opt.has_value()) return false;
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

        return (mouse.x >= left) && (mouse.x <= right) &&
          (mouse.y >= top) && (mouse.y <= bottom);
      }

      //void update_entity(Engine::ECS::Entity entity) override {}
    };

    // System to handle dragging and drawing selected entities
    class GizmoManipulationSystem : public Engine::ECS::System {
    public:
      GizmoManipulationSystem() {
        require_components<Components::Transform2D, Components::Gizmo>();
        set_editor_system(); // Editor system - runs even when paused
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

      void update_entity(Engine::ECS::Entity entity) override {
        auto* tr = entity.get_component<Components::Transform2D>();
        auto* gizmo = entity.get_component<Components::Gizmo>();

        if (!tr || !gizmo) return;

        // if tilemap mode is enabled, remove gizmo and return
        if (TilemapPanel::is_tilemap_mode_enabled()) {
          entity.remove_component<Components::Gizmo>();
          return;
        }

        // UI entities use screen-space coordinates (not affected by camera)
        const auto mouse_opt = entity.has_component<Components::UI>()
          ? Engine::Input::get_mouse_position_if_in_viewport()
          : Engine::Input::get_mouse_world_position_if_in_viewport();

        if (!mouse_opt.has_value()) return;

        const auto mouse = mouse_opt.value();

        // Text entities now use Transform2D position as center and scale as bounds (like regular entities)
        Math::Vec2D rect_center = tr->position;
        Math::Vec2D rect_size = tr->scale;
        const float hw = tr->scale.x * 0.5f, hh = tr->scale.y * 0.5f;
        Math::Vec2D tl = { tr->position.x - hw, tr->position.y - hh };
        Math::Vec2D trc = { tr->position.x + hw, tr->position.y - hh };
        Math::Vec2D br = { tr->position.x + hw, tr->position.y + hh };
        Math::Vec2D bl = { tr->position.x - hw, tr->position.y + hh };

        // Local axes from rotation
        const float ang = tr->rotation * 3.14159265358979323846f / 180.0f;
        const Math::Vec2D xdir{ std::cos(ang), std::sin(ang) };     // local +X
        const Math::Vec2D ydir{ -std::sin(ang), std::cos(ang) };    // local +Y

        // Rotated corners & midpoints
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

        //if (Input::is_pressed(KeyCode::MouseLeft)) {
        //  // inside check via local-space AABB
        //  Math::Vec2D local = rot_deg(mouse, rect_center, -tr->rotation);
        //  bool inside = (local.x >= rect_center.x - hw) && (local.x <= rect_center.x + hw) &&
        //    (local.y >= rect_center.y - hh) && (local.y <= rect_center.y + hh);
        //}

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
            gizmo->is_dragging = true;
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
          }
          else if (!ImGui::GetIO().WantCaptureMouse) {
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
            return;
          }
        }

        // ---------------- Drag ----------------
        const bool held = Engine::Input::is_held(KeyCode::MouseLeft);

        // When mouse is released, send modification event with undo support
        if (!held && gizmo->is_dragging && gizmo->active_handle != Components::GizmoHandleType::None) {
          // Capture final transform state
          Components::Transform2D final_transform = *tr;

          // Capture initial state
          Components::Transform2D initial_transform;
          initial_transform.position = gizmo->start_pos;
          initial_transform.rotation = gizmo->start_rotation;
          initial_transform.scale = gizmo->start_scale;

          // Only record undo if the transform actually changed
          const float epsilon = 0.0001f;
          bool position_changed = std::abs(final_transform.position.x - initial_transform.position.x) > epsilon ||
            std::abs(final_transform.position.y - initial_transform.position.y) > epsilon;
          bool rotation_changed = std::abs(final_transform.rotation - initial_transform.rotation) > epsilon;
          bool scale_changed = std::abs(final_transform.scale.x - initial_transform.scale.x) > epsilon ||
            std::abs(final_transform.scale.y - initial_transform.scale.y) > epsilon;

          if (position_changed || rotation_changed || scale_changed) {
            // Get component type ID
            Engine::ECS::ComponentTypeID type_id = Engine::ECS::ComponentTypeManager::get_type_id<Components::Transform2D>();

            // Convert to GenericValue for the event
            ReflectionLibrary::GenericValue final_data = ReflectionLibrary::to_generic_value(final_transform);
            ReflectionLibrary::GenericValue initial_data = ReflectionLibrary::to_generic_value(initial_transform);

            // Send modification event (this will handle undo/redo and marking dirty)
            Engine::EventManager::send_event<Engine::Events::ModifyComponentEvent>(
              entity.get_id(), type_id, final_data, initial_data, true
            );
          }

          gizmo->is_dragging = false;
          gizmo->active_handle = Components::GizmoHandleType::None;
          return;
        }

        if (!held) {
          gizmo->is_dragging = false;
          gizmo->active_handle = Components::GizmoHandleType::None;
          return;
        }
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
        } break;

        default: break;

        }
      }

      void draw_entity(const Engine::ECS::Entity entity) override {
        auto* tr = entity.get_component<Components::Transform2D>();
        auto* gizmo = entity.get_component<Components::Gizmo>();

        if (!tr || !gizmo) return;

        // UI entities draw gizmos in screen space (not affected by camera)
        const bool is_ui = entity.has_component<Components::UI>();

        // Use UI or world-space draw functions depending on entity type
        auto gizmo_draw_rect_outline = [is_ui](const Math::Vec2D& pos, const Math::Vec2D& size, f32 rot, f32 thick, const Math::Vec4D& col) {
          if (is_ui) Graphics::draw_rect_outline_ui(pos, size, rot, thick, col);
          else Graphics::draw_rect_outline(pos, size, rot, thick, col);
        };
        auto gizmo_draw_circle_outline = [is_ui](const Math::Vec2D& pos, f32 radius, f32 thick, const Math::Vec4D& col, int segments) {
          if (is_ui) Graphics::draw_circle_outline_ui(pos, radius, thick, col, segments);
          else Graphics::draw_circle_outline(pos, radius, thick, col, segments);
        };
        auto gizmo_draw_line = [is_ui](const Math::Vec2D& start, const Math::Vec2D& end, f32 thick, const Math::Vec4D& col) {
          if (is_ui) Graphics::draw_line_ui(start, end, thick, col);
          else Graphics::draw_line(start, end, thick, col);
        };

        // All entities (including text) use Transform2D position as center and scale as bounds
        Math::Vec2D rect_center = tr->position;
        Math::Vec2D rect_size = tr->scale;

        gizmo_draw_rect_outline(
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
            gizmo_draw_rect_outline(c, { gizmo->handle_size, gizmo->handle_size }, tr->rotation, 2.0f, col);
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

          gizmo_draw_circle_outline(center, ring_radius, gizmo->rotate_ring_thickness, ring_col, 64);

          // crosshair aligned to local axes
          const Math::Vec4D colX{ 0.2f, 1.0f, 0.2f, 1.0f };
          const Math::Vec4D colY{ 1.0f, 0.2f, 0.2f, 1.0f };
          gizmo_draw_line({ center.x - xdir.x * ring_radius, center.y - xdir.y * ring_radius },
            { center.x + xdir.x * ring_radius, center.y + xdir.y * ring_radius },
            1.5f, colX);
          gizmo_draw_line({ center.x - ydir.x * ring_radius, center.y - ydir.y * ring_radius },
            { center.x + ydir.x * ring_radius, center.y + ydir.y * ring_radius },
            1.5f, colY);

          // knob outline
          gizmo_draw_circle_outline(knob, gizmo->rotate_handle_radius, gizmo->outline_thickness, ring_col, 24);
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
            gizmo_draw_rect_outline(shaft_center, { len_x, gizmo->arrow_thickness }, tr->rotation, gizmo->arrow_thickness, col);
            const float hs = gizmo->arrow_head_size;
            const Math::Vec2D tip{ from.x + xdir.x * len_x, from.y + xdir.y * len_x };
            gizmo_draw_rect_outline(tip, { hs, gizmo->arrow_thickness }, tr->rotation + 45.0f, gizmo->arrow_thickness, col);
            gizmo_draw_rect_outline(tip, { hs, gizmo->arrow_thickness }, tr->rotation - 45.0f, gizmo->arrow_thickness, col);
          }

          // Y axis
          {
            Math::Vec2D from = mid_top_w;
            Math::Vec2D shaft_center{ from.x - ydir.x * (len_y * 0.5f), from.y - ydir.y * (len_y * 0.5f) };
            Math::Vec4D col = (gizmo->hover_handle == Components::GizmoHandleType::AxisY) ? gizmo->handle_hover_color : gizmo->handle_color;
            gizmo_draw_rect_outline(shaft_center, { len_y, gizmo->arrow_thickness }, tr->rotation + 90.0f, gizmo->arrow_thickness, col);
            const float hs = gizmo->arrow_head_size;
            const Math::Vec2D tip{ from.x - ydir.x * len_y, from.y - ydir.y * len_y };
            gizmo_draw_rect_outline(tip, { hs, gizmo->arrow_thickness }, tr->rotation + 90.0f + 45.0f, gizmo->arrow_thickness, col);
            gizmo_draw_rect_outline(tip, { hs, gizmo->arrow_thickness }, tr->rotation + 90.0f - 45.0f, gizmo->arrow_thickness, col);
          }
        }

        // ---------- ImGui Toolbar ----------
        {
          Components::Gizmo* gizmo_nc = const_cast<Components::Gizmo*>(gizmo);

          Math::Vec2D min_pos = ImGuiManager::get_game_render_area_min();
          ImGui::SetNextWindowPos(ImVec2(min_pos.x + 10.0f, min_pos.y + 10.0f));

          ImGui::SetNextWindowBgAlpha(0.9f);

          if (ImGui::Begin("Gizmo Tools", nullptr,
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse))
          {
            int mode = static_cast<int>(gizmo_nc->mode);

            ImGui::TextUnformatted("Select Tool");
            ImGui::Separator();

            if (ImGui::RadioButton("Grab / Move", mode == 0)) mode = 0;
            if (ImGui::RadioButton("Arrows (X/Y)", mode == 1)) mode = 1;
            if (ImGui::RadioButton("Rotate", mode == 2)) mode = 2;
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
