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
#include "../GUI/ViewportToolbar.h"
#include "../UndoManager/UndoManager.h"
#include "../ProjectSaveManager.h"
#include "GizmoUtils.h"
#include "Precompiled.h"
#include <cfloat>
#include <vector>

namespace Engine
{
  namespace Systems
  {
    // System to add Gizmo component when entities with Transform2D are clicked
    class GizmoSelectionSystem : public Engine::ECS::System {
    public:
      GizmoSelectionSystem() {
        require_components<Components::Transform2D>();
        exclude_components<Components::Text, Components::TileCollider>();
        set_editor_system();
      }

      void update() override {
        // Ctrl+Shift+A: deselect all entities
        if (Engine::Input::is_pressed(KeyCode::A) &&
          (Engine::Input::is_held(KeyCode::CtrlLeft) || Engine::Input::is_held(KeyCode::CtrlRight)) &&
          (Engine::Input::is_held(KeyCode::ShiftLeft) || Engine::Input::is_held(KeyCode::ShiftRight))) {
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
          return;
        }

        bool ctrl_held = Engine::Input::is_held(KeyCode::CtrlLeft) || Engine::Input::is_held(KeyCode::CtrlRight);

        // If an already-selected entity is under the mouse, don't change selection (unless Ctrl for toggle)
        if (!ctrl_held && GizmoUtil::is_any_gizmo_entity_under_mouse()) return;

        Engine::ECS::EntityID best_entity = Engine::ECS::INVALID_ENTITY;
        auto& containers = Engine::ECS::Internal::get_containers();
        for (Engine::ECS::LayerID layer = 0; layer < containers.layer_entities.size(); ++layer) {
          if (!get_layer_signature().test(layer) || !containers.layer_active_state.test(layer)) continue;
          for (Engine::ECS::EntityID entityId : containers.layer_entities[layer]) {
            if (!get_entities().contains(entityId)) continue;
            if (GizmoUtil::is_entity_under_mouse(Engine::ECS::Entity(entityId)))
              best_entity = entityId;
          }
        }

        if (Engine::Input::is_pressed(KeyCode::MouseLeft) && !TilemapPanel::is_tilemap_mode_enabled()) {
          if (best_entity != Engine::ECS::INVALID_ENTITY) {
            Engine::ECS::Entity entity(best_entity);
            if (ctrl_held)
              Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(best_entity, false, true);
            else if (!entity.has_component<Components::Gizmo>())
              Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(best_entity, true);
          }
          else if (!ImGui::GetIO().WantCaptureMouse && !ctrl_held) {
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
          }
        }
      }
    };

    // System to select text entities when clicked
    class GizmoTextSelectionSystem : public Engine::ECS::System {
    public:
      GizmoTextSelectionSystem() {
        require_components<Components::Transform2D, Components::Text, Components::Font>();
        set_editor_system();
      }

      void update() override {
        bool ctrl_held = Engine::Input::is_held(KeyCode::CtrlLeft) || Engine::Input::is_held(KeyCode::CtrlRight);

        // If an already-selected entity is under the mouse, don't change selection (unless Ctrl for toggle)
        if (!ctrl_held && GizmoUtil::is_any_gizmo_entity_under_mouse()) return;

        Engine::ECS::EntityID best_entity = Engine::ECS::INVALID_ENTITY;
        auto& containers = Engine::ECS::Internal::get_containers();
        for (Engine::ECS::LayerID layer = 0; layer < containers.layer_entities.size(); ++layer) {
          if (!get_layer_signature().test(layer) || !containers.layer_active_state.test(layer)) continue;
          for (Engine::ECS::EntityID entityId : containers.layer_entities[layer]) {
            if (!get_entities().contains(entityId)) continue;
            if (is_text_under_mouse(Engine::ECS::Entity(entityId)))
              best_entity = entityId;
          }
        }

        if (Engine::Input::is_pressed(KeyCode::MouseLeft) && !TilemapPanel::is_tilemap_mode_enabled()) {
          if (best_entity != Engine::ECS::INVALID_ENTITY) {
            Engine::ECS::Entity entity(best_entity);
            if (ctrl_held)
              Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(best_entity, false, true);
            else if (!entity.has_component<Components::Gizmo>())
              Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(best_entity, true);
          }
          else if (!ImGui::GetIO().WantCaptureMouse && !ctrl_held) {
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
          }
        }
      }

      // Text entities use measured text dimensions for their bounding box (top-left aligned)
      bool is_text_under_mouse(Engine::ECS::Entity entity) {
        auto* tr = entity.get_component<Components::Transform2D>();
        auto* text = entity.get_component<Components::Text>();
        auto* font = entity.get_component<Components::Font>();
        if (!tr || !text || !font) return false;

        const auto mouse_opt = GizmoUtil::get_mouse_for_entity(entity);
        if (!mouse_opt.has_value()) return false;
        const auto mouse = mouse_opt.value();

        float text_scale = (tr->scale.x + tr->scale.y) * 0.5f;
        Math::Vec2D text_size = Engine::Graphics::measure_text(font->font_handle, text->content, text_scale);

        return (mouse.x >= tr->position.x) && (mouse.x <= tr->position.x + text_size.x) &&
               (mouse.y >= tr->position.y) && (mouse.y <= tr->position.y + text_size.y);
      }
    };

    // System to handle dragging and drawing selected entities
    class GizmoManipulationSystem : public Engine::ECS::System {
    public:
      GizmoManipulationSystem() {
        require_components<Components::Transform2D, Components::Gizmo>();
        set_editor_system();
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
        const float ang = tr->rotation * GizmoUtil::kPi / 180.0f;
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
        const auto toolbar_mode = ViewportToolbar::get_gizmo_mode();
        // When multiple entities are selected, disable per-entity scale handles
        // (GizmoMultiScaleSystem handles unified UI scale; non-UI multi-scale is disabled)
        const bool multi_selected = get_entities().size() > 1;
        // Multi-select interaction handled by GizmoMultiScaleSystem (move, scale, deselect)
        if (multi_selected) return;

        const bool allow_scale = (toolbar_mode == Components::GizmoMode::Scale);
        const bool allow_axes = (toolbar_mode == Components::GizmoMode::ArrowMove);
        const bool allow_body = (toolbar_mode == Components::GizmoMode::GrabMove);
        const bool allow_rot = (toolbar_mode == Components::GizmoMode::Rotate);

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
          if (GizmoUtil::dist_to_segment(mouse, x_a, x_b) <= tol) gizmo->hover_handle = Components::GizmoHandleType::AxisX;
          else if (GizmoUtil::dist_to_segment(mouse, y_a, y_b) <= tol) gizmo->hover_handle = Components::GizmoHandleType::AxisY;
        }
        if (gizmo->hover_handle == Components::GizmoHandleType::None && allow_rot) {
          const float d = GizmoUtil::distance(mouse, ring_center);
          const float pickTol = gizmo->rotate_ring_thickness * 0.5f + 4.0f;
          if (std::abs(d - ring_radius) <= pickTol ||
            GizmoUtil::distance(mouse, knob_center) <= gizmo->rotate_handle_radius + 2.0f)
            gizmo->hover_handle = Components::GizmoHandleType::RotateHandle;
        }
        if (gizmo->hover_handle == Components::GizmoHandleType::None && allow_body) {
          Math::Vec2D local = GizmoUtil::rot_deg(mouse, rect_center, -tr->rotation);
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
          bool position_changed = std::abs(final_transform.position.x - initial_transform.position.x) > GizmoUtil::kEpsilon ||
            std::abs(final_transform.position.y - initial_transform.position.y) > GizmoUtil::kEpsilon;
          bool rotation_changed = std::abs(final_transform.rotation - initial_transform.rotation) > GizmoUtil::kEpsilon;
          bool scale_changed = std::abs(final_transform.scale.x - initial_transform.scale.x) > GizmoUtil::kEpsilon ||
            std::abs(final_transform.scale.y - initial_transform.scale.y) > GizmoUtil::kEpsilon;

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
          float t = GizmoUtil::dot(delta, xdir);                      // project on local X
          tr->position = GizmoUtil::mul_add(gizmo->start_pos, t, xdir);
        } break;

        case Components::GizmoHandleType::AxisY: {
          if (!allow_axes) break;
          Math::Vec2D delta{
            mouse.x - gizmo->start_mouse_world.x, mouse.y - gizmo->start_mouse_world.y
          };
          float t = GizmoUtil::dot(delta, ydir);                      // project on local Y
          tr->position = GizmoUtil::mul_add(gizmo->start_pos, t, ydir);
        } break;

        case Components::GizmoHandleType::RotateHandle: {
          if (!allow_rot) break;
          float ang_now = std::atan2(mouse.y - gizmo->rotate_pivot.y, mouse.x - gizmo->rotate_pivot.x);
          float delta_deg = (ang_now - gizmo->start_angle_rad) * (180.0f / GizmoUtil::kPi);
          if (shift) {
            delta_deg = std::round(delta_deg / GizmoUtil::kSnapAngle) * GizmoUtil::kSnapAngle;
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
          const Math::Vec2D anchor_local = GizmoUtil::rot_deg(gizmo->scale_anchor, rect_center, -tr->rotation);
          const Math::Vec2D mouse_local = GizmoUtil::rot_deg(mouse, rect_center, -tr->rotation);
          Math::Vec2D delta_local{ mouse_local.x - anchor_local.x, mouse_local.y - anchor_local.y };
          Math::Vec2D center_local{ anchor_local.x + delta_local.x * 0.5f,
                                    anchor_local.y + delta_local.y * 0.5f };
          Math::Vec2D size_local{ std::max(GizmoUtil::kMinScale, std::abs(delta_local.x)),
                                  std::max(GizmoUtil::kMinScale, std::abs(delta_local.y)) };
          if (shift) {
            float aspect = (gizmo->start_scale.y != 0.f) ? (gizmo->start_scale.x / gizmo->start_scale.y) : 1.f;
            if (size_local.y > 0.f) size_local.x = std::max(GizmoUtil::kMinScale, std::abs(size_local.y * aspect));
          }
          const Math::Vec2D center_world = GizmoUtil::rot_deg(center_local, rect_center, tr->rotation);
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

        // Convenience aliases for auto-dispatched draw calls
        auto gizmo_draw_rect_outline = [is_ui](const Math::Vec2D& pos, const Math::Vec2D& size, f32 rot, f32 thick, const Math::Vec4D& col) {
          GizmoUtil::draw_rect_outline_auto(is_ui, pos, size, rot, thick, col);
          };
        auto gizmo_draw_circle_outline = [is_ui](const Math::Vec2D& pos, f32 radius, f32 thick, const Math::Vec4D& col, int segments) {
          GizmoUtil::draw_circle_outline_auto(is_ui, pos, radius, thick, col, segments);
          };
        auto gizmo_draw_line = [is_ui](const Math::Vec2D& start, const Math::Vec2D& end, f32 thick, const Math::Vec4D& col) {
          GizmoUtil::draw_line_auto(is_ui, start, end, thick, col);
          };

        // All entities (including text) use Transform2D position as center and scale as bounds
        Math::Vec2D rect_center = tr->position;
        Math::Vec2D rect_size = tr->scale;

        // Read gizmo mode from the viewport toolbar (global state)
        const auto gizmo_mode = ViewportToolbar::get_gizmo_mode();

        // When multiple entities are selected the unified bounding rect (drawn by
        // GizmoMultiScaleSystem) replaces all per-entity outlines.
        const bool multi_sel_draw = get_entities().size() > 1;

        if (!multi_sel_draw) {
          gizmo_draw_rect_outline(
            rect_center,
            rect_size,
            tr->rotation,
            gizmo->outline_thickness,
            gizmo->outline_color
          );
        }

        // Rotated geometry for handles/arrows
        const float ang = tr->rotation * GizmoUtil::kPi / 180.0f;
        const Math::Vec2D xdir{ std::cos(ang), std::sin(ang) };
        const Math::Vec2D ydir{ -std::sin(ang), std::cos(ang) };
        const float hw = rect_size.x * 0.5f, hh = rect_size.y * 0.5f;
        const Math::Vec2D tl_r = { rect_center.x + (-hw) * xdir.x + (-hh) * ydir.x, rect_center.y + (-hw) * xdir.y + (-hh) * ydir.y };
        const Math::Vec2D tr_r = { rect_center.x + (hw)*xdir.x + (-hh) * ydir.x, rect_center.y + (hw)*xdir.y + (-hh) * ydir.y };
        const Math::Vec2D br_r = { rect_center.x + (hw)*xdir.x + (hh)*ydir.x, rect_center.y + (hw)*xdir.y + (hh)*ydir.y };
        const Math::Vec2D bl_r = { rect_center.x + (-hw) * xdir.x + (hh)*ydir.x, rect_center.y + (-hw) * xdir.y + (hh)*ydir.y };
        const Math::Vec2D mid_top_w{ (tl_r.x + tr_r.x) * 0.5f, (tl_r.y + tr_r.y) * 0.5f };
        const Math::Vec2D mid_right_w{ (tr_r.x + br_r.x) * 0.5f, (tr_r.y + br_r.y) * 0.5f };

        // Scale handles — hidden when multiple entities are selected (unified scale instead)
        if (gizmo_mode == Components::GizmoMode::Scale && !multi_sel_draw) {
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
        if (gizmo_mode == Components::GizmoMode::Rotate && !multi_sel_draw) {
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
        if (gizmo_mode == Components::GizmoMode::ArrowMove && !multi_sel_draw) {
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

        // Toolbar is now drawn by ViewportToolbar::draw() inside ViewportPanel

      }
    };

    // =====================================================================
    // System: Unified multi-entity manipulation for selected entities.
    //   When multiple entities are selected, this system provides:
    //   - GrabMove:  body drag (all entities move together)
    //   - ArrowMove: axis-constrained drag along X or Y
    //   - Rotate:    rotate all entities around their centroid
    //   - Scale:     proportional corner-handle scale (UI-only)
    //   Also handles click-empty-space deselect for multi-selection.
    // =====================================================================
    class GizmoMultiScaleSystem : public Engine::ECS::System {
    public:
      GizmoMultiScaleSystem() {
        require_components<Components::Transform2D, Components::Gizmo>();
        set_editor_system();
      }

      // ---- Scale state ----
      enum class MSHandle { None, TL, TR, BR, BL };
      MSHandle  ms_hover = MSHandle::None;
      MSHandle  ms_active = MSHandle::None;
      bool      ms_dragging = false;
      Math::Vec2D ms_anchor;
      Math::Vec2D ms_start_mouse;
      struct EntSnap { Engine::ECS::EntityID id; Math::Vec2D pos; Math::Vec2D scale; };
      std::vector<EntSnap> ms_start_snap;
      Math::Vec2D ms_start_rect_min, ms_start_rect_max;

      // ---- Move state (GrabMove) ----
      bool mm_dragging = false;
      Math::Vec2D mm_start_mouse_screen;
      Math::Vec2D mm_start_mouse_world;
      struct MoveSnap { Engine::ECS::EntityID id; Math::Vec2D pos; Math::Vec2D scale; bool is_ui; };
      std::vector<MoveSnap> mm_snaps;

      // ---- Rotate state ----
      bool mr_dragging = false;
      float mr_start_angle = 0.0f;
      Math::Vec2D mr_centroid{};
      struct RotSnap { Engine::ECS::EntityID id; Math::Vec2D pos; float rotation; };
      std::vector<RotSnap> mr_snaps;

      // ---- ArrowMove state ----
      enum class MAAxis { None, X, Y };
      MAAxis ma_hover = MAAxis::None;
      MAAxis ma_active = MAAxis::None;
      bool ma_dragging = false;
      Math::Vec2D ma_start_mouse_screen;
      Math::Vec2D ma_start_mouse_world;
      std::vector<MoveSnap> ma_snaps;

      // ---- Visual config ----
      static constexpr float ms_handle_size = GizmoUtil::kHandleSize;
      const Math::Vec4D ms_handle_color       = { 0.1f, 0.8f, 1.0f, 1.0f };
      const Math::Vec4D ms_handle_hover_color = { 1.0f, 0.9f, 0.1f, 1.0f };
      const Math::Vec4D ms_rect_color         = { 1.0f, 1.0f, 0.0f, 1.0f };

      // ---- Helper: record undo for position-only changes ----
      void record_move_undo(const std::vector<MoveSnap>& snaps) {
        struct Snap { Engine::ECS::EntityID id; Math::Vec2D init_pos; Math::Vec2D final_pos; };
        std::vector<Snap> finals;
        for (auto& s : snaps) {
          auto ent = Engine::ECS::get_entity(s.id);
          if (!ent.is_valid() || !ent.has_component<Components::Transform2D>()) continue;
          auto* t = ent.get_component<Components::Transform2D>();
          if (std::abs(t->position.x - s.pos.x) > GizmoUtil::kEpsilon ||
              std::abs(t->position.y - s.pos.y) > GizmoUtil::kEpsilon) {
            finals.push_back({ s.id, s.pos, t->position });
          }
        }
        if (finals.empty()) return;
        Engine::UndoManager::record(
          [finals]() {
            for (auto& f : finals) {
              auto e = Engine::ECS::get_entity(f.id);
              if (e.is_valid() && e.has_component<Components::Transform2D>())
                e.get_component<Components::Transform2D>()->position = f.final_pos;
            }
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          },
          [finals]() {
            for (auto& f : finals) {
              auto e = Engine::ECS::get_entity(f.id);
              if (e.is_valid() && e.has_component<Components::Transform2D>())
                e.get_component<Components::Transform2D>()->position = f.init_pos;
            }
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          }
        );
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
      }

      // ---- Helper: snapshot all entities for move ----
      void snapshot_move(const Engine::ECS::SparseSet<Engine::ECS::EntityID>& ents,
                         std::vector<MoveSnap>& out) {
        out.clear();
        for (auto eid : ents) {
          Engine::ECS::Entity e(eid);
          auto* t = e.get_component<Components::Transform2D>();
          if (t) out.push_back({ eid, t->position, t->scale, e.has_component<Components::UI>() });
        }
      }

      // ---- Helper: apply delta-move to snapped entities ----
      void apply_move_delta(const std::vector<MoveSnap>& snaps,
                            const Math::Vec2D& mouse_screen, const Math::Vec2D& mouse_world,
                            const Math::Vec2D& start_screen, const Math::Vec2D& start_world) {
        for (auto& s : snaps) {
          auto ent = Engine::ECS::get_entity(s.id);
          if (!ent.is_valid()) continue;
          auto* t = ent.get_component<Components::Transform2D>();
          if (!t) continue;
          if (s.is_ui) {
            t->position.x = s.pos.x + (mouse_screen.x - start_screen.x);
            t->position.y = s.pos.y + (mouse_screen.y - start_screen.y);
          } else {
            t->position.x = s.pos.x + (mouse_world.x - start_world.x);
            t->position.y = s.pos.y + (mouse_world.y - start_world.y);
          }
        }
      }

      void reset_all_state() {
        ms_dragging = false; ms_active = MSHandle::None; ms_hover = MSHandle::None;
        mm_dragging = false;
        mr_dragging = false;
        ma_dragging = false; ma_active = MAAxis::None; ma_hover = MAAxis::None;
      }

      void update() override {
        const auto& ents = get_entities();
        if (ents.size() < 2) { reset_all_state(); return; }

        const auto mode = ViewportToolbar::get_gizmo_mode();

        // Get both mouse coordinate spaces
        auto screen_opt = Engine::Input::get_mouse_position_if_in_viewport();
        auto world_opt  = Engine::Input::get_mouse_world_position_if_in_viewport();
        if (!screen_opt || !world_opt) {
          if (!Engine::Input::is_held(KeyCode::MouseLeft)) reset_all_state();
          return;
        }
        auto mouse_screen = screen_opt.value();
        auto mouse_world  = world_opt.value();

        // Check if any selected entity is under the mouse (rotation-aware)
        bool any_entity_under_mouse = GizmoUtil::is_any_entity_under_mouse(ents, mouse_screen, mouse_world);

        const bool just_pressed = Engine::Input::is_pressed(KeyCode::MouseLeft);
        const bool ctrl  = Engine::Input::is_held(KeyCode::CtrlLeft)  || Engine::Input::is_held(KeyCode::CtrlRight);
        const bool shift = Engine::Input::is_held(KeyCode::ShiftLeft) || Engine::Input::is_held(KeyCode::ShiftRight);
        const bool held  = Engine::Input::is_held(KeyCode::MouseLeft);

        // ===== GrabMove mode =====
        if (mode == Components::GizmoMode::GrabMove) {
          ms_dragging = false; ms_active = MSHandle::None; mr_dragging = false; ma_dragging = false;

          if (just_pressed && !ImGui::GetIO().WantCaptureMouse && any_entity_under_mouse && !ctrl) {
            mm_dragging = true;
            mm_start_mouse_screen = mouse_screen;
            mm_start_mouse_world  = mouse_world;
            snapshot_move(ents, mm_snaps);
          }

          if (!held && mm_dragging) {
            record_move_undo(mm_snaps);
            mm_dragging = false;
            return;
          }
          if (!held) mm_dragging = false;

          if (mm_dragging)
            apply_move_delta(mm_snaps, mouse_screen, mouse_world, mm_start_mouse_screen, mm_start_mouse_world);

          // Click empty space to deselect
          if (just_pressed && !ImGui::GetIO().WantCaptureMouse && !any_entity_under_mouse && !ctrl && !mm_dragging)
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);

          return;
        }

        mm_dragging = false; // reset move state for other modes

        // ===== ArrowMove mode =====
        if (mode == Components::GizmoMode::ArrowMove) {
          ms_dragging = false; ms_active = MSHandle::None; mr_dragging = false;

          // Compute AABB for axis arrow geometry
          auto aabb = GizmoUtil::compute_aabb(ents);
          if (!aabb.valid) return;
          Math::Vec2D center = GizmoUtil::aabb_center(aabb);
          Math::Vec2D sz     = GizmoUtil::aabb_size(aabb);

          // Arrow endpoints (right edge + extra, top edge + extra)
          const float extra = 30.0f;
          const float arrow_len_x = sz.x * 0.5f + extra;
          const float arrow_len_y = sz.y * 0.5f + extra;
          Math::Vec2D x_a = { center.x + sz.x * 0.5f, center.y };
          Math::Vec2D x_b = { center.x + sz.x * 0.5f + arrow_len_x, center.y };
          Math::Vec2D y_a = { center.x, center.y - sz.y * 0.5f };
          Math::Vec2D y_b = { center.x, center.y - sz.y * 0.5f - arrow_len_y };

          const float tol = 8.0f;

          // Hover
          if (!ma_dragging) {
            ma_hover = MAAxis::None;
            // Use appropriate mouse coordinate for hit testing
            const auto& mouse = aabb.all_ui ? mouse_screen : mouse_world;
            if (GizmoUtil::dist_to_segment(mouse, x_a, x_b) <= tol) ma_hover = MAAxis::X;
            else if (GizmoUtil::dist_to_segment(mouse, y_a, y_b) <= tol) ma_hover = MAAxis::Y;
          }

          // Press
          if (just_pressed && ma_hover != MAAxis::None) {
            ma_active = ma_hover;
            ma_dragging = true;
            ma_start_mouse_screen = mouse_screen;
            ma_start_mouse_world  = mouse_world;
            snapshot_move(ents, ma_snaps);
          }

          // Release
          if (!held && ma_dragging) {
            record_move_undo(ma_snaps);
            ma_dragging = false;
            ma_active = MAAxis::None;
            return;
          }
          if (!held) { ma_dragging = false; ma_active = MAAxis::None; }

          // Drag: axis-constrained movement
          if (ma_dragging) {
            for (auto& s : ma_snaps) {
              auto ent = Engine::ECS::get_entity(s.id);
              if (!ent.is_valid()) continue;
              auto* t = ent.get_component<Components::Transform2D>();
              if (!t) continue;
              if (s.is_ui) {
                if (ma_active == MAAxis::X) t->position.x = s.pos.x + (mouse_screen.x - ma_start_mouse_screen.x);
                if (ma_active == MAAxis::Y) t->position.y = s.pos.y + (mouse_screen.y - ma_start_mouse_screen.y);
              } else {
                if (ma_active == MAAxis::X) t->position.x = s.pos.x + (mouse_world.x - ma_start_mouse_world.x);
                if (ma_active == MAAxis::Y) t->position.y = s.pos.y + (mouse_world.y - ma_start_mouse_world.y);
              }
            }
          }

          // Click empty space to deselect
          if (just_pressed && !ImGui::GetIO().WantCaptureMouse && !any_entity_under_mouse &&
              ma_hover == MAAxis::None && !ctrl && !ma_dragging)
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);

          return;
        }

        ma_dragging = false; ma_active = MAAxis::None;

        // ===== Rotate mode =====
        if (mode == Components::GizmoMode::Rotate) {
          ms_dragging = false; ms_active = MSHandle::None;

          // Compute centroid of all selected entities
          Math::Vec2D centroid{ 0, 0 };
          int count = 0;
          for (auto eid : ents) {
            Engine::ECS::Entity e(eid);
            auto* t = e.get_component<Components::Transform2D>();
            if (!t) continue;
            centroid.x += t->position.x;
            centroid.y += t->position.y;
            ++count;
          }
          if (count > 0) { centroid.x /= count; centroid.y /= count; }

          // Use AABB to determine if all are UI for coordinate space
          auto aabb = GizmoUtil::compute_aabb(ents);
          const auto& mouse = aabb.all_ui ? mouse_screen : mouse_world;

          // Ring geometry
          Math::Vec2D bsz = GizmoUtil::aabb_size(aabb);
          float ring_radius = 0.5f * std::max(bsz.x, bsz.y) + 20.0f;
          float d = GizmoUtil::distance(mouse, centroid);
          float pick_tol = 8.0f;
          bool over_ring = std::abs(d - ring_radius) <= pick_tol;

          // Press
          if (just_pressed && (over_ring || any_entity_under_mouse) && !ctrl) {
            mr_dragging = true;
            mr_centroid = centroid;
            mr_start_angle = std::atan2(mouse.y - centroid.y, mouse.x - centroid.x);
            mr_snaps.clear();
            for (auto eid : ents) {
              Engine::ECS::Entity e(eid);
              auto* t = e.get_component<Components::Transform2D>();
              if (t) mr_snaps.push_back({ eid, t->position, t->rotation });
            }
          }

          // Release
          if (!held && mr_dragging) {
            // Record undo for rotation + position changes
            struct RUSnap { Engine::ECS::EntityID id; Math::Vec2D init_pos; float init_rot; Math::Vec2D final_pos; float final_rot; };
            std::vector<RUSnap> finals;
            for (auto& s : mr_snaps) {
              auto e = Engine::ECS::get_entity(s.id);
              if (!e.is_valid() || !e.has_component<Components::Transform2D>()) continue;
              auto* t = e.get_component<Components::Transform2D>();
              if (std::abs(t->position.x - s.pos.x) > GizmoUtil::kEpsilon ||
                  std::abs(t->position.y - s.pos.y) > GizmoUtil::kEpsilon ||
                  std::abs(t->rotation - s.rotation) > GizmoUtil::kEpsilon) {
                finals.push_back({ s.id, s.pos, s.rotation, t->position, t->rotation });
              }
            }
            if (!finals.empty()) {
              Engine::UndoManager::record(
                [finals]() {
                  for (auto& f : finals) {
                    auto e = Engine::ECS::get_entity(f.id);
                    if (!e.is_valid() || !e.has_component<Components::Transform2D>()) continue;
                    auto* t = e.get_component<Components::Transform2D>();
                    t->position = f.final_pos;
                    t->rotation = f.final_rot;
                  }
                  Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
                },
                [finals]() {
                  for (auto& f : finals) {
                    auto e = Engine::ECS::get_entity(f.id);
                    if (!e.is_valid() || !e.has_component<Components::Transform2D>()) continue;
                    auto* t = e.get_component<Components::Transform2D>();
                    t->position = f.init_pos;
                    t->rotation = f.init_rot;
                  }
                  Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
                }
              );
              Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
            }
            mr_dragging = false;
            return;
          }
          if (!held) mr_dragging = false;

          // Drag: rotate all entities around centroid
          if (mr_dragging) {
            float ang_now = std::atan2(mouse.y - mr_centroid.y, mouse.x - mr_centroid.x);
            float delta_deg = (ang_now - mr_start_angle) * (180.0f / GizmoUtil::kPi);
            if (shift) delta_deg = std::round(delta_deg / GizmoUtil::kSnapAngle) * GizmoUtil::kSnapAngle;

            for (auto& s : mr_snaps) {
              auto ent = Engine::ECS::get_entity(s.id);
              if (!ent.is_valid()) continue;
              auto* t = ent.get_component<Components::Transform2D>();
              if (!t) continue;
              // Rotate position around centroid
              t->position = GizmoUtil::rot_deg(s.pos, mr_centroid, delta_deg);
              // Add delta to entity's own rotation
              t->rotation = s.rotation + delta_deg;
            }
          }

          // Click empty space to deselect
          if (just_pressed && !ImGui::GetIO().WantCaptureMouse && !any_entity_under_mouse &&
              !over_ring && !ctrl && !mr_dragging)
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);

          return;
        }

        mr_dragging = false;

        // ===== Scale mode =====
        // Scale only works when ALL selected entities are UI (screen-space)
        auto aabb = GizmoUtil::compute_aabb(ents);
        if (!aabb.valid || !aabb.all_ui) { ms_dragging = false; ms_active = MSHandle::None; return; }

        Math::Vec2D tl = aabb.min;
        Math::Vec2D tr_c{ aabb.max.x, aabb.min.y };
        Math::Vec2D br = aabb.max;
        Math::Vec2D bl{ aabb.min.x, aabb.max.y };

        const auto mouse = mouse_screen;

        // ---- hover ----
        ms_hover = MSHandle::None;
        float pr = ms_handle_size * 0.6f;
        if (GizmoUtil::point_near(mouse, tl,   pr)) ms_hover = MSHandle::TL;
        else if (GizmoUtil::point_near(mouse, tr_c, pr)) ms_hover = MSHandle::TR;
        else if (GizmoUtil::point_near(mouse, br,   pr)) ms_hover = MSHandle::BR;
        else if (GizmoUtil::point_near(mouse, bl,   pr)) ms_hover = MSHandle::BL;

        // ---- press ----
        if (just_pressed && ms_hover != MSHandle::None) {
          ms_active = ms_hover;
          ms_dragging = true;
          ms_start_mouse = mouse;
          ms_start_rect_min = aabb.min;
          ms_start_rect_max = aabb.max;
          switch (ms_active) {
          case MSHandle::TL: ms_anchor = br;   break;
          case MSHandle::TR: ms_anchor = bl;   break;
          case MSHandle::BR: ms_anchor = tl;   break;
          case MSHandle::BL: ms_anchor = tr_c; break;
          default: break;
          }
          ms_start_snap.clear();
          for (auto eid : ents) {
            Engine::ECS::Entity e(eid);
            auto* t = e.get_component<Components::Transform2D>();
            if (t) ms_start_snap.push_back({ eid, t->position, t->scale });
          }
        }

        // ---- release / undo ----
        if (!held && ms_dragging && ms_active != MSHandle::None) {
          struct ScaleSnap { Engine::ECS::EntityID id; Math::Vec2D init_pos; Math::Vec2D init_scale; Math::Vec2D final_pos; Math::Vec2D final_scale; };
          std::vector<ScaleSnap> finals;
          for (auto& snap : ms_start_snap) {
            auto e = Engine::ECS::get_entity(snap.id);
            if (!e.is_valid() || !e.has_component<Components::Transform2D>()) continue;
            auto* t = e.get_component<Components::Transform2D>();
            if (std::abs(t->position.x - snap.pos.x) > GizmoUtil::kEpsilon ||
                std::abs(t->position.y - snap.pos.y) > GizmoUtil::kEpsilon ||
                std::abs(t->scale.x - snap.scale.x) > GizmoUtil::kEpsilon ||
                std::abs(t->scale.y - snap.scale.y) > GizmoUtil::kEpsilon) {
              finals.push_back({ snap.id, snap.pos, snap.scale, t->position, t->scale });
            }
          }
          if (!finals.empty()) {
            Engine::UndoManager::record(
              [finals]() {
                for (auto& f : finals) {
                  auto e = Engine::ECS::get_entity(f.id);
                  if (!e.is_valid() || !e.has_component<Components::Transform2D>()) continue;
                  auto* t = e.get_component<Components::Transform2D>();
                  t->position = f.final_pos;
                  t->scale = f.final_scale;
                }
                Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
              },
              [finals]() {
                for (auto& f : finals) {
                  auto e = Engine::ECS::get_entity(f.id);
                  if (!e.is_valid() || !e.has_component<Components::Transform2D>()) continue;
                  auto* t = e.get_component<Components::Transform2D>();
                  t->position = f.init_pos;
                  t->scale = f.init_scale;
                }
                Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
              }
            );
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          }
          ms_dragging = false;
          ms_active = MSHandle::None;
          return;
        }

        if (!held) { ms_dragging = false; ms_active = MSHandle::None; }

        if (ms_dragging && ms_active != MSHandle::None) {
          float new_minX = std::min(ms_anchor.x, mouse.x);
          float new_minY = std::min(ms_anchor.y, mouse.y);
          float new_maxX = std::max(ms_anchor.x, mouse.x);
          float new_maxY = std::max(ms_anchor.y, mouse.y);
          float new_w = std::max(GizmoUtil::kMinScale, new_maxX - new_minX);
          float new_h = std::max(GizmoUtil::kMinScale, new_maxY - new_minY);

          if (shift) {
            float old_w = ms_start_rect_max.x - ms_start_rect_min.x;
            float old_h = ms_start_rect_max.y - ms_start_rect_min.y;
            float aspect = (old_h > 0.001f) ? (old_w / old_h) : 1.0f;
            new_w = std::max(GizmoUtil::kMinScale, new_h * aspect);
            if (mouse.x < ms_anchor.x) new_minX = ms_anchor.x - new_w;
            else new_maxX = ms_anchor.x + new_w;
          }

          float old_w = ms_start_rect_max.x - ms_start_rect_min.x;
          float old_h = ms_start_rect_max.y - ms_start_rect_min.y;
          if (old_w < 0.001f) old_w = 1.0f;
          if (old_h < 0.001f) old_h = 1.0f;
          float sx = new_w / old_w;
          float sy = new_h / old_h;

          for (auto& snap : ms_start_snap) {
            Engine::ECS::Entity e = Engine::ECS::get_entity(snap.id);
            if (!e.is_valid()) continue;
            auto* t = e.get_component<Components::Transform2D>();
            if (!t) continue;
            float nx = (old_w > 0.001f) ? (snap.pos.x - ms_start_rect_min.x) / old_w : 0.5f;
            float ny = (old_h > 0.001f) ? (snap.pos.y - ms_start_rect_min.y) / old_h : 0.5f;
            t->position.x = new_minX + nx * new_w;
            t->position.y = new_minY + ny * new_h;
            t->scale.x = snap.scale.x * sx;
            t->scale.y = snap.scale.y * sy;
          }
        }

        // Click empty space to deselect
        if (just_pressed && !ImGui::GetIO().WantCaptureMouse && !any_entity_under_mouse &&
            ms_hover == MSHandle::None && !ctrl && !ms_dragging)
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
      }

      void draw() override {
        const auto& ents = get_entities();
        if (ents.size() < 2) return;

        auto aabb = GizmoUtil::compute_aabb(ents);
        if (!aabb.valid) return;

        // Draw unified bounding rect
        GizmoUtil::draw_bounding_rect(aabb, ms_rect_color);

        const auto mode = ViewportToolbar::get_gizmo_mode();

        // Scale handles (UI-only)
        if (mode == Components::GizmoMode::Scale && aabb.all_ui) {
          Math::Vec2D tl = aabb.min;
          Math::Vec2D tr_c{ aabb.max.x, aabb.min.y };
          Math::Vec2D br = aabb.max;
          Math::Vec2D bl{ aabb.min.x, aabb.max.y };
          auto draw_ms_handle = [&](const Math::Vec2D& c, MSHandle h) {
            Math::Vec4D col = (ms_hover == h || ms_active == h) ? ms_handle_hover_color : ms_handle_color;
            GizmoUtil::draw_rect_outline_auto(true, c, { ms_handle_size, ms_handle_size }, 0.0f, 2.0f, col);
          };
          draw_ms_handle(tl,   MSHandle::TL);
          draw_ms_handle(tr_c, MSHandle::TR);
          draw_ms_handle(br,   MSHandle::BR);
          draw_ms_handle(bl,   MSHandle::BL);
        }

        // Rotation ring + crosshair
        if (mode == Components::GizmoMode::Rotate) {
          // Centroid
          Math::Vec2D centroid{ 0, 0 };
          int count = 0;
          for (auto eid : ents) {
            Engine::ECS::Entity e(eid);
            auto* t = e.get_component<Components::Transform2D>();
            if (!t) continue;
            centroid.x += t->position.x;
            centroid.y += t->position.y;
            ++count;
          }
          if (count > 0) { centroid.x /= count; centroid.y /= count; }

          Math::Vec2D sz = GizmoUtil::aabb_size(aabb);
          float ring_radius = 0.5f * std::max(sz.x, sz.y) + 20.0f;

          const Math::Vec4D ring_col = mr_dragging ? ms_handle_hover_color : ms_handle_color;
          GizmoUtil::draw_circle_outline_auto(aabb.all_ui, centroid, ring_radius, 2.0f, ring_col, 64);

          // Crosshair axes
          const Math::Vec4D colX{ 0.2f, 1.0f, 0.2f, 1.0f };
          const Math::Vec4D colY{ 1.0f, 0.2f, 0.2f, 1.0f };
          GizmoUtil::draw_line_auto(aabb.all_ui, { centroid.x - ring_radius, centroid.y }, { centroid.x + ring_radius, centroid.y }, 1.5f, colX);
          GizmoUtil::draw_line_auto(aabb.all_ui, { centroid.x, centroid.y - ring_radius }, { centroid.x, centroid.y + ring_radius }, 1.5f, colY);
        }

        // Axis arrows
        if (mode == Components::GizmoMode::ArrowMove) {
          Math::Vec2D center = GizmoUtil::aabb_center(aabb);
          Math::Vec2D sz     = GizmoUtil::aabb_size(aabb);
          const float extra = 30.0f;
          const float arrow_len_x = sz.x * 0.5f + extra;
          const float arrow_len_y = sz.y * 0.5f + extra;
          const float thick = 3.0f;

          // X axis arrow (right)
          {
            Math::Vec2D from{ center.x + sz.x * 0.5f, center.y };
            Math::Vec2D to  { from.x + arrow_len_x, center.y };
            Math::Vec4D col = (ma_hover == MAAxis::X || ma_active == MAAxis::X) ? ms_handle_hover_color : Math::Vec4D{ 0.2f, 1.0f, 0.2f, 1.0f };
            GizmoUtil::draw_line_auto(aabb.all_ui, from, to, thick, col);
            // Arrowhead
            GizmoUtil::draw_line_auto(aabb.all_ui, to, { to.x - 8.0f, to.y - 5.0f }, thick, col);
            GizmoUtil::draw_line_auto(aabb.all_ui, to, { to.x - 8.0f, to.y + 5.0f }, thick, col);
          }

          // Y axis arrow (up)
          {
            Math::Vec2D from{ center.x, center.y - sz.y * 0.5f };
            Math::Vec2D to  { center.x, from.y - arrow_len_y };
            Math::Vec4D col = (ma_hover == MAAxis::Y || ma_active == MAAxis::Y) ? ms_handle_hover_color : Math::Vec4D{ 1.0f, 0.2f, 0.2f, 1.0f };
            GizmoUtil::draw_line_auto(aabb.all_ui, from, to, thick, col);
            // Arrowhead
            GizmoUtil::draw_line_auto(aabb.all_ui, to, { to.x - 5.0f, to.y + 8.0f }, thick, col);
            GizmoUtil::draw_line_auto(aabb.all_ui, to, { to.x + 5.0f, to.y + 8.0f }, thick, col);
          }
        }
      }
    };

  } // namespace Systems
} // namespace Engine
