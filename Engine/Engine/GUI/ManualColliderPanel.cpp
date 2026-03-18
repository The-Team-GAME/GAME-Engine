//-----------------------------------------------------------------------------------
// file:    ManualColliderPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of ManualColliderPanel. Lets the user place AABB / OBB
//          collider entities in the world and edit them with the standard gizmo.
//          AABB colliders have rotation locked to 0. Scale changes from the gizmo
//          are synced back to BoxCollider dimensions each frame.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "../Precompiled.h"
#include "ManualColliderPanel.h"
#include "LucideIcons.h"
#include "ViewportToolbar.h"
#include "../ECS/ECS.h"
#include "../Components/CollisionComponents.h"
#include "../Components/SpatialComponents.h"
#include "../Components/GizmoComponents.h"
#include "../Components/PhysicsComponents.h"
#include "../Graphics/Renderer.h"
#include "../InputManager.h"
#include "../ProjectSaveManager.h"
#include "../LevelManager.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include "../Math/Vector2D.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cmath>

namespace ManualColliderPanel {

  namespace {

    Engine::ECS::EntityID selected_id = Engine::ECS::INVALID_ENTITY;
    std::string placement_layer = "default";

    // Scale-change tracking: Transform2D.scale → BoxCollider.width/height
    std::unordered_map<Engine::ECS::EntityID, Math::Vec2D> prev_scales;

    // Rotation-change tracking: for OBB polygon sync
    std::unordered_map<Engine::ECS::EntityID, float> prev_rotations;

    // ---- Visual constants ----
    constexpr float OUTLINE_THICK = 2.0f;
    constexpr float SEL_THICK = 3.5f;
    const Math::Vec4D COLOR_OUTLINE = { 1.0f, 0.0f, 1.0f, 1.0f };   // Magenta
    const Math::Vec4D COLOR_SEL_OUTLINE = { 1.0f, 0.55f, 1.0f, 1.0f };  // Bright magenta

    // ---- Helpers ----
    auto& get_layer_names() {
      return Engine::ECS::Internal::get_containers().layer_id_to_name;
    }

    void mark_dirty() {
      Engine::ProjectSaveManager::set_dirty(
        Engine::ProjectSaveManager::DirtyStates::Level(
          Engine::LevelManager::get_current_level_path()));
    }

    // Propagate Transform2D.scale → BoxCollider dimensions.
    void sync_collider_from_scale(Engine::ECS::Entity entity,
      Components::Transform2D* tr,
      Components::BoxCollider* col) {
      col->width = tr->scale.x;
      col->height = tr->scale.y;
      col->aabb.half_width = tr->scale.x * 0.5f;
      col->aabb.half_height = tr->scale.y * 0.5f;
      if (col->type == Components::ColliderType::OBB) {
        col->polygon = Polygon::create_box(col->width, col->height);
        col->polygon.rotation = tr->rotation;
      }
      prev_scales[entity.get_id()] = tr->scale;
    }

    // Ensure tr->scale matches collider bounds so the gizmo renders correctly.
    void force_sync_scale(Engine::ECS::Entity entity,
      Components::Transform2D* tr,
      Components::BoxCollider* col) {
      auto* gizmo = entity.get_component<Components::Gizmo>();
      if (gizmo && gizmo->is_dragging) return; // let the scale gizmo run freely
      constexpr float eps = 0.01f;
      if (std::abs(tr->scale.x - col->width) > eps ||
        std::abs(tr->scale.y - col->height) > eps) {
        tr->scale = { col->width, col->height };
        prev_scales[entity.get_id()] = tr->scale;
      }
    }

    // Draw gizmo handles for a selected ManualCollider entity using collider
    // dimensions directly so the gizmo is always correctly sized regardless of
    // tr->scale sync timing.
    void draw_gizmo_tools(Engine::ECS::Entity entity,
      Components::Transform2D* tr,
      Components::BoxCollider* col) {
      auto* gizmo = entity.get_component<Components::Gizmo>();
      if (!gizmo) return;

      constexpr float kPi = 3.14159265358979323846f;

      const Math::Vec2D center = tr->position;
      const float hw = col->width * 0.5f;
      const float hh = col->height * 0.5f;
      const float ang = tr->rotation * kPi / 180.0f;
      const Math::Vec2D xdir = { std::cos(ang),  std::sin(ang) };   // local +X
      const Math::Vec2D ydir = { -std::sin(ang), std::cos(ang) };   // local +Y

      // Rotated corners & edge midpoints
      const Math::Vec2D tl_r = { center.x + (-hw) * xdir.x + (-hh) * ydir.x,
                                  center.y + (-hw) * xdir.y + (-hh) * ydir.y };
      const Math::Vec2D tr_r = { center.x + (hw)*xdir.x + (-hh) * ydir.x,
                                  center.y + (hw)*xdir.y + (-hh) * ydir.y };
      const Math::Vec2D br_r = { center.x + (hw)*xdir.x + (hh)*ydir.x,
                                  center.y + (hw)*xdir.y + (hh)*ydir.y };
      const Math::Vec2D bl_r = { center.x + (-hw) * xdir.x + (hh)*ydir.x,
                                  center.y + (-hw) * xdir.y + (hh)*ydir.y };
      const Math::Vec2D mid_top_r = { (tl_r.x + tr_r.x) * 0.5f, (tl_r.y + tr_r.y) * 0.5f };
      const Math::Vec2D mid_right_r = { (tr_r.x + br_r.x) * 0.5f, (tr_r.y + br_r.y) * 0.5f };

      // Selection outline (green, using collider dims)
      Engine::Graphics::draw_rect_outline(center, { col->width, col->height },
        tr->rotation, gizmo->outline_thickness, gizmo->outline_color);

      const auto mode = ViewportToolbar::get_gizmo_mode();

      // ---- Scale handles ----
      if (mode == Components::GizmoMode::Scale) {
        auto draw_handle = [&](const Math::Vec2D& c, bool hovered) {
          const Math::Vec4D hcol = hovered ? gizmo->handle_hover_color : gizmo->handle_color;
          Engine::Graphics::draw_rect_outline(c,
            { gizmo->handle_size, gizmo->handle_size }, tr->rotation, 2.0f, hcol);
          };
        draw_handle(tl_r, gizmo->hover_handle == Components::GizmoHandleType::ScaleTL);
        draw_handle(tr_r, gizmo->hover_handle == Components::GizmoHandleType::ScaleTR);
        draw_handle(br_r, gizmo->hover_handle == Components::GizmoHandleType::ScaleBR);
        draw_handle(bl_r, gizmo->hover_handle == Components::GizmoHandleType::ScaleBL);
      }

      // ---- Rotation ring + crosshair + knob ----
      if (mode == Components::GizmoMode::Rotate) {
        const float ring_r = 0.5f * std::max(col->width, col->height) + gizmo->rotate_ring_padding;
        const Math::Vec2D knob = { center.x - ydir.x * (ring_r + gizmo->rotate_knob_offset),
                                   center.y - ydir.y * (ring_r + gizmo->rotate_knob_offset) };
        const Math::Vec4D ring_col = (gizmo->hover_handle == Components::GizmoHandleType::RotateHandle)
          ? gizmo->handle_hover_color : gizmo->handle_color;

        Engine::Graphics::draw_circle_outline(center, ring_r, gizmo->rotate_ring_thickness, ring_col, 64);

        const Math::Vec4D colX{ 0.2f, 1.0f, 0.2f, 1.0f };
        const Math::Vec4D colY{ 1.0f, 0.2f, 0.2f, 1.0f };
        Engine::Graphics::draw_line(
          { center.x - xdir.x * ring_r, center.y - xdir.y * ring_r },
          { center.x + xdir.x * ring_r, center.y + xdir.y * ring_r }, 1.5f, colX);
        Engine::Graphics::draw_line(
          { center.x - ydir.x * ring_r, center.y - ydir.y * ring_r },
          { center.x + ydir.x * ring_r, center.y + ydir.y * ring_r }, 1.5f, colY);

        Engine::Graphics::draw_circle_outline(knob, gizmo->rotate_handle_radius,
          gizmo->outline_thickness, ring_col, 24);
      }

      // ---- Axis arrows ----
      if (mode == Components::GizmoMode::ArrowMove) {
        const float len_x = std::max(12.0f, col->width * gizmo->arrow_length_factor) + gizmo->arrow_outside_extra;
        const float len_y = std::max(12.0f, col->height * gizmo->arrow_length_factor) + gizmo->arrow_outside_extra;

        // X axis
        {
          const Math::Vec2D shaft_c = { mid_right_r.x + xdir.x * (len_x * 0.5f),
                                        mid_right_r.y + xdir.y * (len_x * 0.5f) };
          const Math::Vec4D ac = (gizmo->hover_handle == Components::GizmoHandleType::AxisX)
            ? gizmo->handle_hover_color : gizmo->handle_color;
          Engine::Graphics::draw_rect_outline(shaft_c, { len_x, gizmo->arrow_thickness },
            tr->rotation, gizmo->arrow_thickness, ac);
          const float hs = gizmo->arrow_head_size;
          const Math::Vec2D tip = { mid_right_r.x + xdir.x * len_x, mid_right_r.y + xdir.y * len_x };
          Engine::Graphics::draw_rect_outline(tip, { hs, gizmo->arrow_thickness }, tr->rotation + 45.0f, gizmo->arrow_thickness, ac);
          Engine::Graphics::draw_rect_outline(tip, { hs, gizmo->arrow_thickness }, tr->rotation - 45.0f, gizmo->arrow_thickness, ac);
        }

        // Y axis
        {
          const Math::Vec2D shaft_c = { mid_top_r.x - ydir.x * (len_y * 0.5f),
                                        mid_top_r.y - ydir.y * (len_y * 0.5f) };
          const Math::Vec4D ac = (gizmo->hover_handle == Components::GizmoHandleType::AxisY)
            ? gizmo->handle_hover_color : gizmo->handle_color;
          Engine::Graphics::draw_rect_outline(shaft_c, { len_y, gizmo->arrow_thickness },
            tr->rotation + 90.0f, gizmo->arrow_thickness, ac);
          const float hs = gizmo->arrow_head_size;
          const Math::Vec2D tip = { mid_top_r.x - ydir.x * len_y, mid_top_r.y - ydir.y * len_y };
          Engine::Graphics::draw_rect_outline(tip, { hs, gizmo->arrow_thickness }, tr->rotation + 90.0f + 45.0f, gizmo->arrow_thickness, ac);
          Engine::Graphics::draw_rect_outline(tip, { hs, gizmo->arrow_thickness }, tr->rotation + 90.0f - 45.0f, gizmo->arrow_thickness, ac);
        }
      }
    }

    // Create a new ManualCollider entity at the camera center.
    Engine::ECS::Entity spawn_collider(Components::ColliderType ctype,
      const std::string& layer_name) {
      auto bounds = Engine::Graphics::get_camera_bounds();
      Math::Vec2D pos = { (bounds.min_x + bounds.max_x) * 0.5f,
                             (bounds.min_y + bounds.max_y) * 0.5f };
      constexpr float grid = 64.0f;
      Math::Vec2D scale = { grid, grid };

      auto entity = Engine::ECS::create_entity()
        .add_component<Components::ManualCollider>()
        .add_component<Components::Transform2D>(pos, 0.0f, scale)
        .add_component<Components::RigidBody>(true, false);

      if (ctype == Components::ColliderType::AABB) {
        entity.add_component<Components::BoxCollider>(grid, grid, true);
      }
      else { // OBB
        entity.add_component<Components::BoxCollider>(grid, grid, 0.0f, true);
      }

      entity.clear_layers();
      entity.add_to_layer(layer_name);
      prev_scales[entity.get_id()] = scale;
      return entity;
    }

  } // anonymous namespace

  // ===========================================================================
  // draw_panel
  // ===========================================================================
  void draw_panel() {
#ifndef GAME_BUILD
    ImGui::Begin("Manual Colliders");

    // ---- Layer selector ----
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Layer:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180.0f);
    const auto& layer_names = get_layer_names();
    if (ImGui::BeginCombo("##mc_layer", placement_layer.c_str())) {
      for (Engine::ECS::LayerID lid = 0;
        lid < static_cast<Engine::ECS::LayerID>(layer_names.size()); ++lid) {
        const std::string& lname = layer_names[lid];
        if (lname.empty()) continue;
        bool sel = (lname == placement_layer);
        if (ImGui::Selectable(lname.c_str(), sel))
          placement_layer = lname;
        if (sel) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    ImGui::Separator();

    // ---- Creation buttons ----
    if (ImGui::Button("+ AABB")) {
      auto e = spawn_collider(Components::ColliderType::AABB, placement_layer);
      selected_id = e.get_id();
      Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(selected_id);
      mark_dirty();
    }
    ImGui::SameLine();
    if (ImGui::Button("+ OBB")) {
      auto e = spawn_collider(Components::ColliderType::OBB, placement_layer);
      selected_id = e.get_id();
      Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(selected_id);
      mark_dirty();
    }

    ImGui::Separator();

    // ---- Collider entity list ----
    ImGui::Text("Colliders in scene:");
    ImGui::BeginChild("mc_list", ImVec2(0, 180), true);

    for (auto& entity : Engine::ECS::get_entities_with<Components::ManualCollider>()) {
      auto* col = entity.get_component<Components::BoxCollider>();
      const char* type_str = col
        ? (col->type == Components::ColliderType::AABB ? "AABB" : "OBB")
        : "?";

      char label[64];
      snprintf(label, sizeof(label), "[%s] Entity %u", type_str,
        static_cast<unsigned>(entity.get_id()));

      bool is_sel = (entity.get_id() == selected_id);
      ImGui::PushID(static_cast<int>(entity.get_id()));
      if (ImGui::Selectable(label, is_sel)) {
        selected_id = entity.get_id();
        Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(selected_id);
      }

      if (ImGui::BeginPopupContextItem("mc_ctx")) {
        if (ImGui::MenuItem("Delete")) {
          Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(
            entity.get_id(), true);
          if (selected_id == entity.get_id())
            selected_id = Engine::ECS::INVALID_ENTITY;
        }
        if (ImGui::MenuItem("Duplicate")) {
          auto prefab = Engine::ECS::entity_to_prefab(entity);
          auto dup = Engine::ECS::instantiate_prefab(prefab);
          if (auto* dtr = dup.get_component<Components::Transform2D>())
            dtr->position.x += 64.0f;
          selected_id = dup.get_id();
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(selected_id);
          if (auto* dtr = dup.get_component<Components::Transform2D>())
            prev_scales[dup.get_id()] = dtr->scale;
          mark_dirty();
        }
        ImGui::EndPopup();
      }
      ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::Separator();

    // ---- Property editor ----
    Engine::ECS::Entity sel(selected_id);
    if (!sel.is_valid() || !sel.has_component<Components::ManualCollider>()) {
      ImGui::TextDisabled("Select a collider to edit its properties.");
      ImGui::End();
      return;
    }

    auto* tr = sel.get_component<Components::Transform2D>();
    auto* col = sel.get_component<Components::BoxCollider>();
    if (!tr || !col) { ImGui::End(); return; }

    ImGui::Text("Entity %u", static_cast<unsigned>(selected_id));
    ImGui::Separator();

    // Layer assignment
    {
      const auto& layer_names = get_layer_names();
      auto layers = sel.get_layers();

      // Find the first layer this entity is on
      std::string cur_layer;
      for (Engine::ECS::LayerID lid = 0;
        lid < static_cast<Engine::ECS::LayerID>(layer_names.size()); ++lid) {
        if (!layer_names[lid].empty() && layers.test(lid)) {
          cur_layer = layer_names[lid];
          break;
        }
      }

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Layer:");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(180.0f);
      if (ImGui::BeginCombo("##mc_sel_layer", cur_layer.c_str())) {
        for (Engine::ECS::LayerID lid = 0;
          lid < static_cast<Engine::ECS::LayerID>(layer_names.size()); ++lid) {
          const std::string& lname = layer_names[lid];
          if (lname.empty()) continue;
          bool is_cur = (lname == cur_layer);
          if (ImGui::Selectable(lname.c_str(), is_cur) && lname != cur_layer) {
            sel.clear_layers();
            sel.add_to_layer(lname);
            mark_dirty();
          }
          if (is_cur) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
    }

    ImGui::Separator();

    // Position
    float pos_arr[2] = { tr->position.x, tr->position.y };
    if (ImGui::DragFloat2("Position##mc", pos_arr, 1.0f)) {
      tr->position = { pos_arr[0], pos_arr[1] };
      mark_dirty();
    }

    // Rotation — hidden and locked for AABB
    if (col->type == Components::ColliderType::OBB) {
      if (ImGui::DragFloat("Rotation##mc", &tr->rotation, 0.5f)) {
        col->polygon.rotation = tr->rotation;
        mark_dirty();
      }
    }
    else {
      ImGui::TextDisabled("Rotation: locked (AABB)");
    }

    // Type switcher (AABB ↔ OBB only)
    int type_idx = (col->type == Components::ColliderType::AABB) ? 0 : 1;
    const char* type_names[] = { "AABB", "OBB" };
    if (ImGui::Combo("Type##mc", &type_idx, type_names, 2)) {
      float w = col->width > 0.0f ? col->width : 64.0f;
      float h = col->height > 0.0f ? col->height : 64.0f;
      if (type_idx == 0) {
        col->type = Components::ColliderType::AABB;
        col->aabb = AABB(w, h);
        tr->rotation = 0.0f;
      }
      else {
        col->type = Components::ColliderType::OBB;
        col->polygon = Polygon::create_box(w, h);
        col->polygon.rotation = tr->rotation;
      }
      mark_dirty();
    }

    ImGui::Separator();

    // Size
    float dims[2] = { col->width, col->height };
    if (ImGui::DragFloat2("Size (W/H)##mc", dims, 0.5f, 0.1f, 50000.0f)) {
      col->width = dims[0];
      col->height = dims[1];
      col->aabb.half_width = dims[0] * 0.5f;
      col->aabb.half_height = dims[1] * 0.5f;
      if (col->type == Components::ColliderType::OBB) {
        col->polygon = Polygon::create_box(col->width, col->height);
        col->polygon.rotation = tr->rotation;
      }
      tr->scale = { col->width, col->height };
      prev_scales[selected_id] = tr->scale;
      mark_dirty();
    }

    // Offset
    float off[2] = { col->offset.x, col->offset.y };
    if (ImGui::DragFloat2("Offset##mc", off, 0.5f)) {
      col->offset = { off[0], off[1] };
      mark_dirty();
    }

    ImGui::Separator();

    ImGui::DragFloat("Kinetic Friction##mc", &col->kinetic_friction, 0.01f, 0.0f, 2.0f);
    ImGui::DragFloat("Static Friction##mc", &col->static_friction, 0.01f, 0.0f, 2.0f);
    ImGui::Checkbox("Resolve Collisions##mc", &col->resolve_collisions);

    ImGui::End();
#endif
  }

  // ===========================================================================
  // draw_overlay — draws magenta outlines for all ManualCollider entities
  // ===========================================================================
  void draw_overlay() {
#ifndef GAME_BUILD
    for (auto& entity : Engine::ECS::get_entities_with<Components::ManualCollider>()) {
      auto* tr = entity.get_component<Components::Transform2D>();
      auto* col = entity.get_component<Components::BoxCollider>();
      if (!tr || !col) continue;

      bool        is_sel = (entity.get_id() == selected_id);
      Math::Vec4D color = is_sel ? COLOR_SEL_OUTLINE : COLOR_OUTLINE;
      float       thick = is_sel ? SEL_THICK : OUTLINE_THICK;
      Math::Vec2D center = tr->position + col->offset;

      if (col->type == Components::ColliderType::AABB) {
        Engine::Graphics::draw_rect_outline(
          center, { col->width, col->height }, 0.0f, thick, color);
      }
      else { // OBB
        Engine::Graphics::draw_rect_outline(
          center, { col->width, col->height }, tr->rotation, thick, color);
      }

      // Draw gizmo handles for the selected entity (uses collider dims, not tr->scale)
      if (is_sel && entity.has_component<Components::Gizmo>())
        draw_gizmo_tools(entity, tr, col);
    }
#endif
  }

  // ===========================================================================
  // update — syncs collider geometry from gizmo transforms, locks AABB rotation
  // ===========================================================================
  void update() {
#ifndef GAME_BUILD
    // Sync selected_id when a ManualCollider entity gains the gizmo
    Engine::ECS::for_each_entity_with<Components::Gizmo>([](Engine::ECS::Entity e) {
      if (e.has_component<Components::ManualCollider>())
        selected_id = e.get_id();
      });

    for (auto& entity : Engine::ECS::get_entities_with<Components::ManualCollider>()) {
      auto* tr = entity.get_component<Components::Transform2D>();
      auto* col = entity.get_component<Components::BoxCollider>();
      if (!tr || !col) continue;

      // Lock rotation for AABB
      if (col->type == Components::ColliderType::AABB && std::abs(tr->rotation) > 0.001f)
        tr->rotation = 0.0f;

      // Ensure tr->scale always reflects collider dimensions (fixes gizmo visuals)
      force_sync_scale(entity, tr, col);

      // Rotation sync for OBB polygon
      if (col->type == Components::ColliderType::OBB) {
        auto rot_it = prev_rotations.find(entity.get_id());
        if (rot_it == prev_rotations.end()) {
          prev_rotations[entity.get_id()] = tr->rotation;
        }
        else {
          constexpr float eps = 0.001f;
          if (std::abs(rot_it->second - tr->rotation) > eps) {
            col->polygon.rotation = tr->rotation;
            rot_it->second = tr->rotation;
          }
        }
      }

      // Scale sync (AABB / OBB): gizmo scale tool changes tr->scale → sync to BoxCollider
      auto it = prev_scales.find(entity.get_id());
      if (it == prev_scales.end()) {
        prev_scales[entity.get_id()] = tr->scale;
        continue;
      }
      constexpr float eps = 0.001f;
      Math::Vec2D& prev = it->second;
      if (std::abs(prev.x - tr->scale.x) > eps || std::abs(prev.y - tr->scale.y) > eps)
        sync_collider_from_scale(entity, tr, col);
    }
#endif
  }

} // namespace ManualColliderPanel