//-----------------------------------------------------------------------------------
// file:    ManualColliderPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of ManualColliderPanel. Manages a "Manual Collider Mode"
//          that lets the user place AABB / OBB / Polygon collider entities in the
//          world and edit them via a Pointer tool (vertex / resize-handle dragging)
//          or the standard gizmo (move / rotate / scale).
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
#include "../KeyCodes.h"
#include "../ProjectSaveManager.h"
#include "../LevelManager.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include "../Math/Vector2D.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace ManualColliderPanel {

  // ---------------------------------------------------------------------------
  // Internal state
  // ---------------------------------------------------------------------------
  namespace {

    bool mode_enabled = false;
    bool pointer_tool_active = false;

    Engine::ECS::EntityID selected_id = Engine::ECS::INVALID_ENTITY;

    // Layer to assign newly created entities to
    std::string placement_layer = "default";

    // Scale-change tracking for AABB/OBB sync (Transform2D.scale → BoxCollider.width/height)
    std::unordered_map<Engine::ECS::EntityID, Math::Vec2D> prev_scales;

    // Rotation-change tracking for OBB polygon sync
    std::unordered_map<Engine::ECS::EntityID, float> prev_rotations;

    // Pointer tool drag state — Polygon vertex editing
    int drag_vertex_idx = -1;

    // Pointer tool drag state — AABB/OBB resize handles
    // Handles:  0=L, 1=R, 2=T, 3=B,  4=TL, 5=TR, 6=BL, 7=BR
    int drag_handle_idx = -1;

    // ---- Visual constants ----
    constexpr float VERTEX_RADIUS = 7.0f;   // world-unit radius for vertex circles
    constexpr float HANDLE_SIZE = 10.0f;  // world-unit square handle size
    constexpr float CLICK_RADIUS = 14.0f;  // pick radius in world units
    constexpr float OUTLINE_THICK = 2.0f;
    constexpr float SEL_THICK = 3.5f;

    const Math::Vec4D COLOR_OUTLINE = { 1.0f, 0.0f, 1.0f, 1.0f };  // Magenta
    const Math::Vec4D COLOR_SEL_OUTLINE = { 1.0f, 0.55f, 1.0f, 1.0f };  // Bright magenta
    const Math::Vec4D COLOR_HANDLE = { 1.0f, 0.9f,  0.0f, 0.9f };  // Yellow
    const Math::Vec4D COLOR_HANDLE_HOV = { 1.0f, 1.0f,  1.0f, 1.0f };  // White (hovered)
    const Math::Vec4D COLOR_EDGE_MID = { 0.0f, 1.0f,  1.0f, 0.7f };  // Cyan (edge-midpoint insert)

    // ---- Helper: dist2 between two Vec2D ----
    float dist2(Math::Vec2D a, Math::Vec2D b) {
      float dx = a.x - b.x, dy = a.y - b.y;
      return dx * dx + dy * dy;
    }

    // ---- Helper: compute world-space positions of the 8 AABB/OBB handles ----
    void compute_box_handles(Components::Transform2D* tr, Components::BoxCollider* col,
      Math::Vec2D out[8]) {
      float hw = col->width * 0.5f;
      float hh = col->height * 0.5f;
      Math::Vec2D center = tr->position + col->offset;

      // Local-space handle points (symmetric around center)
      Math::Vec2D local[8] = {
        { -hw,   0 },   // 0 L
        {  hw,   0 },   // 1 R
        {   0,  hh },   // 2 T
        {   0, -hh },   // 3 B
        { -hw,  hh },   // 4 TL
        {  hw,  hh },   // 5 TR
        { -hw, -hh },   // 6 BL
        {  hw, -hh },   // 7 BR
      };

      for (int i = 0; i < 8; ++i) {
        out[i] = rotate(local[i], tr->rotation) + center;
      }
    }

    // ---- Helper: world-space vertices for Polygon collider ----
    std::vector<Math::Vec2D> world_poly_verts(Components::Transform2D* tr,
      Components::BoxCollider* col) {
      std::vector<Math::Vec2D> wv;
      wv.reserve(col->polygon.vertices.size());
      for (const auto& v : col->polygon.vertices) {
        wv.push_back(rotate(v, tr->rotation) + tr->position + col->offset);
      }
      return wv;
    }

    // ---- Helper: get ECS layer containers (for layer name list) ----
    auto& get_layer_names() {
      return Engine::ECS::Internal::get_containers().layer_id_to_name;
    }

    // ---- Helper: mark level dirty ----
    void mark_dirty() {
      Engine::ProjectSaveManager::set_dirty(
        Engine::ProjectSaveManager::DirtyStates::Level(
          Engine::LevelManager::get_current_level_path()));
    }

    // ---- Helper: sync BoxCollider dims from changed scale ----
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

    // ---- Helper: ensure Transform2D.scale matches collider bounds ----
    // Called every frame to guarantee the gizmo system sees correct dimensions.
    void force_sync_scale(Engine::ECS::Entity entity,
      Components::Transform2D* tr,
      Components::BoxCollider* col) {
      // Skip if the gizmo is actively being dragged (scale tool changes tr->scale)
      auto* gizmo = entity.get_component<Components::Gizmo>();
      if (gizmo && gizmo->is_dragging) return;

      float want_w, want_h;
      if (col->type == Components::ColliderType::POLYGON && !col->polygon.vertices.empty()) {
        float minX = col->polygon.vertices[0].x, maxX = minX;
        float minY = col->polygon.vertices[0].y, maxY = minY;
        for (const auto& v : col->polygon.vertices) {
          if (v.x < minX) minX = v.x; if (v.x > maxX) maxX = v.x;
          if (v.y < minY) minY = v.y; if (v.y > maxY) maxY = v.y;
        }
        want_w = maxX - minX;
        want_h = maxY - minY;
      }
      else {
        want_w = col->width;
        want_h = col->height;
      }
      constexpr float eps = 0.01f;
      if (std::abs(tr->scale.x - want_w) > eps || std::abs(tr->scale.y - want_h) > eps) {
        tr->scale = { want_w, want_h };
        prev_scales[entity.get_id()] = tr->scale;
      }
    }

    // ---- Gizmo visual constants (match GizmoComponents defaults) ----
    constexpr float GIZMO_HANDLE_SZ       = 10.0f;
    constexpr float GIZMO_ARROW_LEN_FAC   = 0.6f;
    constexpr float GIZMO_ARROW_EXTRA     = 18.0f;
    constexpr float GIZMO_ARROW_HEAD      = 12.0f;
    constexpr float GIZMO_ARROW_THICK     = 3.0f;
    constexpr float GIZMO_RING_PAD        = 10.0f;
    constexpr float GIZMO_RING_THICK      = 2.0f;
    constexpr float GIZMO_KNOB_RADIUS     = 8.0f;
    constexpr float GIZMO_KNOB_OFFSET     = 8.0f;
    const Math::Vec4D GIZMO_COL_NORMAL    = { 0.1f, 0.8f, 1.0f, 1.0f };
    const Math::Vec4D GIZMO_COL_HOVER     = { 1.0f, 0.9f, 0.1f, 1.0f };
    const Math::Vec4D GIZMO_COL_OUTLINE   = { 0.0f, 1.0f, 0.0f, 1.0f };
    const Math::Vec4D GIZMO_COL_X_AXIS    = { 0.2f, 1.0f, 0.2f, 1.0f };
    const Math::Vec4D GIZMO_COL_Y_AXIS    = { 1.0f, 0.2f, 0.2f, 1.0f };
    constexpr float kPi = 3.14159265f;

    // ---- Helper: draw gizmo tool visuals for a ManualCollider entity ----
    void draw_gizmo_tools(Components::Transform2D* tr, Components::BoxCollider* col,
      Engine::ECS::Entity entity) {
      auto gizmo_mode = ViewportToolbar::get_gizmo_mode();
      auto* gizmo = entity.get_component<Components::Gizmo>();

      // Compute effective half-extents from collider
      float hw, hh;
      Math::Vec2D center = tr->position + col->offset;
      if (col->type == Components::ColliderType::POLYGON && !col->polygon.vertices.empty()) {
        float minX = col->polygon.vertices[0].x, maxX = minX;
        float minY = col->polygon.vertices[0].y, maxY = minY;
        for (const auto& v : col->polygon.vertices) {
          if (v.x < minX) minX = v.x; if (v.x > maxX) maxX = v.x;
          if (v.y < minY) minY = v.y; if (v.y > maxY) maxY = v.y;
        }
        hw = (maxX - minX) * 0.5f;
        hh = (maxY - minY) * 0.5f;
      }
      else {
        hw = col->width * 0.5f;
        hh = col->height * 0.5f;
      }

      // Local axes from rotation
      float ang = tr->rotation * kPi / 180.0f;
      Math::Vec2D xdir = { std::cos(ang), std::sin(ang) };
      Math::Vec2D ydir = { -std::sin(ang), std::cos(ang) };

      // Rotated corners
      Math::Vec2D tl_r = { center.x + (-hw)*xdir.x + (-hh)*ydir.x, center.y + (-hw)*xdir.y + (-hh)*ydir.y };
      Math::Vec2D tr_r = { center.x + (hw)*xdir.x + (-hh)*ydir.x,  center.y + (hw)*xdir.y + (-hh)*ydir.y };
      Math::Vec2D br_r = { center.x + (hw)*xdir.x + (hh)*ydir.x,   center.y + (hw)*xdir.y + (hh)*ydir.y };
      Math::Vec2D bl_r = { center.x + (-hw)*xdir.x + (hh)*ydir.x,  center.y + (-hw)*xdir.y + (hh)*ydir.y };
      Math::Vec2D mid_top   = { (tl_r.x + tr_r.x)*0.5f, (tl_r.y + tr_r.y)*0.5f };
      Math::Vec2D mid_right = { (tr_r.x + br_r.x)*0.5f, (tr_r.y + br_r.y)*0.5f };

      // Green selection outline (matches standard gizmo)
      Engine::Graphics::draw_rect_outline(center, { hw*2, hh*2 }, tr->rotation, 2.0f, GIZMO_COL_OUTLINE);

      // ---- Scale mode: 4 corner handles ----
      if (gizmo_mode == Components::GizmoMode::Scale) {
        auto draw_handle = [&](Math::Vec2D pos, Components::GizmoHandleType ht) {
          bool hovered = gizmo && gizmo->hover_handle == ht;
          Math::Vec4D c = hovered ? GIZMO_COL_HOVER : GIZMO_COL_NORMAL;
          Engine::Graphics::draw_rect_outline(pos, { GIZMO_HANDLE_SZ, GIZMO_HANDLE_SZ }, tr->rotation, 2.0f, c);
        };
        draw_handle(tl_r, Components::GizmoHandleType::ScaleTL);
        draw_handle(tr_r, Components::GizmoHandleType::ScaleTR);
        draw_handle(br_r, Components::GizmoHandleType::ScaleBR);
        draw_handle(bl_r, Components::GizmoHandleType::ScaleBL);
      }

      // ---- Rotate mode: ring + crosshair + knob ----
      if (gizmo_mode == Components::GizmoMode::Rotate) {
        float ring_radius = std::max(hw, hh) + GIZMO_RING_PAD;
        bool rot_hover = gizmo && gizmo->hover_handle == Components::GizmoHandleType::RotateHandle;
        Math::Vec4D ring_col = rot_hover ? GIZMO_COL_HOVER : GIZMO_COL_NORMAL;

        Engine::Graphics::draw_circle_outline(center, ring_radius, GIZMO_RING_THICK, ring_col, 64);

        // Crosshair (local axes)
        Engine::Graphics::draw_line(
          { center.x - xdir.x*ring_radius, center.y - xdir.y*ring_radius },
          { center.x + xdir.x*ring_radius, center.y + xdir.y*ring_radius },
          1.5f, GIZMO_COL_X_AXIS);
        Engine::Graphics::draw_line(
          { center.x - ydir.x*ring_radius, center.y - ydir.y*ring_radius },
          { center.x + ydir.x*ring_radius, center.y + ydir.y*ring_radius },
          1.5f, GIZMO_COL_Y_AXIS);

        // Knob
        Math::Vec2D knob = { center.x - ydir.x*(ring_radius + GIZMO_KNOB_OFFSET),
                             center.y - ydir.y*(ring_radius + GIZMO_KNOB_OFFSET) };
        Engine::Graphics::draw_circle_outline(knob, GIZMO_KNOB_RADIUS, 2.0f, ring_col, 24);
      }

      // ---- ArrowMove mode: X and Y axis arrows ----
      if (gizmo_mode == Components::GizmoMode::ArrowMove) {
        float len_x = std::max(12.0f, hw*2 * GIZMO_ARROW_LEN_FAC) + GIZMO_ARROW_EXTRA;
        float len_y = std::max(12.0f, hh*2 * GIZMO_ARROW_LEN_FAC) + GIZMO_ARROW_EXTRA;

        // X axis
        {
          bool hov = gizmo && gizmo->hover_handle == Components::GizmoHandleType::AxisX;
          Math::Vec4D c = hov ? GIZMO_COL_HOVER : GIZMO_COL_NORMAL;
          Math::Vec2D from = mid_right;
          Math::Vec2D shaft = { from.x + xdir.x*(len_x*0.5f), from.y + xdir.y*(len_x*0.5f) };
          Engine::Graphics::draw_rect_outline(shaft, { len_x, GIZMO_ARROW_THICK }, tr->rotation, GIZMO_ARROW_THICK, c);
          Math::Vec2D tip = { from.x + xdir.x*len_x, from.y + xdir.y*len_x };
          Engine::Graphics::draw_rect_outline(tip, { GIZMO_ARROW_HEAD, GIZMO_ARROW_THICK }, tr->rotation + 45.0f, GIZMO_ARROW_THICK, c);
          Engine::Graphics::draw_rect_outline(tip, { GIZMO_ARROW_HEAD, GIZMO_ARROW_THICK }, tr->rotation - 45.0f, GIZMO_ARROW_THICK, c);
        }

        // Y axis
        {
          bool hov = gizmo && gizmo->hover_handle == Components::GizmoHandleType::AxisY;
          Math::Vec4D c = hov ? GIZMO_COL_HOVER : GIZMO_COL_NORMAL;
          Math::Vec2D from = mid_top;
          Math::Vec2D shaft = { from.x - ydir.x*(len_y*0.5f), from.y - ydir.y*(len_y*0.5f) };
          Engine::Graphics::draw_rect_outline(shaft, { len_y, GIZMO_ARROW_THICK }, tr->rotation + 90.0f, GIZMO_ARROW_THICK, c);
          Math::Vec2D tip = { from.x - ydir.x*len_y, from.y - ydir.y*len_y };
          Engine::Graphics::draw_rect_outline(tip, { GIZMO_ARROW_HEAD, GIZMO_ARROW_THICK }, tr->rotation + 90.0f + 45.0f, GIZMO_ARROW_THICK, c);
          Engine::Graphics::draw_rect_outline(tip, { GIZMO_ARROW_HEAD, GIZMO_ARROW_THICK }, tr->rotation + 90.0f - 45.0f, GIZMO_ARROW_THICK, c);
        }
      }
    }

    // ---- Helper: create a ManualCollider entity ----
    Engine::ECS::Entity spawn_collider(Components::ColliderType ctype,
      const std::string& layer_name) {
      // Place at the center of the current view (not at the camera's top-left origin)
      auto bounds = Engine::Graphics::get_camera_bounds();
      Math::Vec2D pos = { (bounds.min_x + bounds.max_x) * 0.5f,
                          (bounds.min_y + bounds.max_y) * 0.5f };
      float       grid = 64.0f;
      Math::Vec2D scale = { grid, grid };

      auto entity = Engine::ECS::create_entity()
        .add_component<Components::ManualCollider>()
        .add_component<Components::Transform2D>(pos, 0.0f, scale)
        .add_component<Components::RigidBody>(true, false); // is_static=true, no gravity

      if (ctype == Components::ColliderType::AABB) {
        entity.add_component<Components::BoxCollider>(grid, grid, true);
      }
      else if (ctype == Components::ColliderType::OBB) {
        entity.add_component<Components::BoxCollider>(grid, grid, 0.0f, true);
      }
      else { // POLYGON
        float hw = grid * 0.5f, hh = grid * 0.5f;
        std::vector<Math::Vec2D> verts = {
          { -hw, -hh }, { hw, -hh }, { hw, hh }, { -hw, hh }
        };
        entity.add_component<Components::BoxCollider>(verts, true);
      }

      entity.clear_layers();
      entity.add_to_layer(layer_name);

      // Initialise prev_scale tracking
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

    // ---- Mode toggle ----
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Manual Collider Mode:");
    ImGui::SameLine();
    if (ImGui::Checkbox("##mode_toggle", &mode_enabled)) {
      if (!mode_enabled) {
        pointer_tool_active = false;
        drag_vertex_idx = -1;
        drag_handle_idx = -1;
      }
    }
    if (mode_enabled) {
      ImGui::SameLine();
      ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, "ACTIVE");
    }

    if (!mode_enabled) {
      ImGui::End();
      return;
    }

    ImGui::Separator();

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
    ImGui::SameLine();
    if (ImGui::Button("+ Polygon")) {
      auto e = spawn_collider(Components::ColliderType::POLYGON, placement_layer);
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
      const char* type_str = "?";
      if (col) {
        if (col->type == Components::ColliderType::AABB)    type_str = "AABB";
        else if (col->type == Components::ColliderType::OBB)     type_str = "OBB";
        else                                                      type_str = "Poly";
      }

      char label[64];
      snprintf(label, sizeof(label), "[%s] Entity %u", type_str,
        static_cast<unsigned>(entity.get_id()));

      bool is_sel = (entity.get_id() == selected_id);
      ImGui::PushID(static_cast<int>(entity.get_id()));
      if (ImGui::Selectable(label, is_sel)) {
        selected_id = entity.get_id();
        Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(selected_id);
      }

      // Context menu: delete / duplicate
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
          prev_scales[dup.get_id()] = dup.get_component<Components::Transform2D>()
            ? dup.get_component<Components::Transform2D>()->scale
            : Math::Vec2D{ 64.0f, 64.0f };
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

    ImGui::Text("Selected Entity %u", static_cast<unsigned>(selected_id));
    ImGui::Separator();

    // Position
    float pos_arr[2] = { tr->position.x, tr->position.y };
    if (ImGui::DragFloat2("Position##mc", pos_arr, 1.0f)) {
      tr->position = { pos_arr[0], pos_arr[1] };
      mark_dirty();
    }

    // Rotation
    if (ImGui::DragFloat("Rotation##mc", &tr->rotation, 0.5f)) {
      if (col->type == Components::ColliderType::OBB)
        col->polygon.rotation = tr->rotation;
      mark_dirty();
    }

    // Collider type switcher
    int type_idx = (col->type == Components::ColliderType::AABB) ? 0
      : (col->type == Components::ColliderType::OBB) ? 1
      : 2;
    const char* type_names[] = { "AABB", "OBB", "Polygon" };
    if (ImGui::Combo("Type##mc", &type_idx, type_names, 3)) {
      float w = col->width > 0.0f ? col->width : 64.0f;
      float h = col->height > 0.0f ? col->height : 64.0f;
      if (type_idx == 0) {
        col->type = Components::ColliderType::AABB;
        col->aabb = AABB(w, h);
      }
      else if (type_idx == 1) {
        col->type = Components::ColliderType::OBB;
        col->polygon = Polygon::create_box(w, h);
        col->polygon.rotation = tr->rotation;
      }
      else {
        col->type = Components::ColliderType::POLYGON;
        float hw = w * 0.5f, hh = h * 0.5f;
        col->polygon.vertices = { { -hw, -hh }, { hw, -hh }, { hw, hh }, { -hw, hh } };
        col->polygon.rotation = 0.0f;
      }
      mark_dirty();
    }

    ImGui::Separator();

    // ---- Type-specific properties ----
    if (col->type == Components::ColliderType::AABB ||
      col->type == Components::ColliderType::OBB) {
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
      float off[2] = { col->offset.x, col->offset.y };
      if (ImGui::DragFloat2("Offset##mc", off, 0.5f)) {
        col->offset = { off[0], off[1] };
        mark_dirty();
      }

    }
    else { // POLYGON
      if (ImGui::Button("+ Add Vertex##mc")) {
        col->polygon.vertices.push_back({ 0.0f, 0.0f });
        mark_dirty();
      }
      ImGui::Text("Vertices (%zu):", col->polygon.vertices.size());
      int to_delete = -1;
      for (int i = 0; i < static_cast<int>(col->polygon.vertices.size()); ++i) {
        float v[2] = { col->polygon.vertices[i].x, col->polygon.vertices[i].y };
        char vlabel[16];
        snprintf(vlabel, sizeof(vlabel), "V%d##mcv", i);
        ImGui::PushID(i);
        ImGui::SetNextItemWidth(140.0f);
        if (ImGui::DragFloat2(vlabel, v, 0.5f)) {
          col->polygon.vertices[i] = { v[0], v[1] };
          mark_dirty();
        }
        ImGui::SameLine();
        if (col->polygon.vertices.size() > 3 && ImGui::SmallButton("X##mcdv"))
          to_delete = i;
        ImGui::PopID();
      }
      if (to_delete >= 0) {
        col->polygon.vertices.erase(col->polygon.vertices.begin() + to_delete);
        mark_dirty();
      }
    }

    ImGui::Separator();

    // Friction + resolve
    ImGui::DragFloat("Kinetic Friction##mc", &col->kinetic_friction, 0.01f, 0.0f, 2.0f);
    ImGui::DragFloat("Static Friction##mc", &col->static_friction, 0.01f, 0.0f, 2.0f);
    ImGui::Checkbox("Resolve Collisions##mc", &col->resolve_collisions);

    ImGui::End();
#endif
  }

  // ===========================================================================
  // draw_overlay
  // ===========================================================================
  void draw_overlay() {
#ifndef GAME_BUILD
    // Draw when mode is active OR when debug layer is visible (F3)
    bool debug_on = Engine::ECS::is_layer_active("debug");
    bool should_draw = mode_enabled || debug_on;
    if (!should_draw) return;

    for (auto& entity : Engine::ECS::get_entities_with<Components::ManualCollider>()) {
      auto* tr = entity.get_component<Components::Transform2D>();
      auto* col = entity.get_component<Components::BoxCollider>();
      if (!tr || !col) continue;

      bool      is_sel = (entity.get_id() == selected_id);
      Math::Vec4D color = is_sel ? COLOR_SEL_OUTLINE : COLOR_OUTLINE;
      float     thick = is_sel ? SEL_THICK : OUTLINE_THICK;

      // ---- Collider outline ----
      if (col->type == Components::ColliderType::AABB) {
        Engine::Graphics::draw_rect_outline(
          tr->position + col->offset,
          { col->width, col->height },
          0.0f, thick, color);

      }
      else if (col->type == Components::ColliderType::OBB) {
        Engine::Graphics::draw_rect_outline(
          tr->position + col->offset,
          { col->width, col->height },
          tr->rotation, thick, color);

      }
      else if (!col->polygon.vertices.empty()) { // POLYGON
        Engine::Graphics::draw_polygon_outline(
          col->polygon.vertices,
          tr->position + col->offset,
          tr->rotation, thick, color);
      }

      // ---- Gizmo tool visuals (selected entity, non-pointer mode) ----
      if (is_sel && mode_enabled && !pointer_tool_active && entity.has_component<Components::Gizmo>()) {
        draw_gizmo_tools(tr, col, entity);
      }

      // ---- Pointer-tool handles (selected entity only) ----
      if (!is_sel || !pointer_tool_active || !mode_enabled) continue;

      if (col->type == Components::ColliderType::POLYGON) {
        auto wv = world_poly_verts(tr, col);

        // Vertex handles (circles)
        for (int i = 0; i < static_cast<int>(wv.size()); ++i) {
          Math::Vec4D hcol = (i == drag_vertex_idx) ? COLOR_HANDLE_HOV : COLOR_HANDLE;
          Engine::Graphics::draw_circle_outline(wv[i], VERTEX_RADIUS, 1.5f, hcol);
        }

        // Edge-midpoint handles (smaller cyan circles, for insertion)
        for (int i = 0; i < static_cast<int>(wv.size()); ++i) {
          int j = (i + 1) % static_cast<int>(wv.size());
          Math::Vec2D mid = { (wv[i].x + wv[j].x) * 0.5f,
                              (wv[i].y + wv[j].y) * 0.5f };
          Engine::Graphics::draw_circle_outline(mid, VERTEX_RADIUS * 0.55f, 1.0f, COLOR_EDGE_MID);
        }

      }
      else {
        // AABB / OBB: 8 square handles
        Math::Vec2D handles[8];
        compute_box_handles(tr, col, handles);
        for (int i = 0; i < 8; ++i) {
          Math::Vec4D hcol = (i == drag_handle_idx) ? COLOR_HANDLE_HOV : COLOR_HANDLE;
          Engine::Graphics::draw_rect_outline(
            handles[i], { HANDLE_SIZE, HANDLE_SIZE },
            tr->rotation, 1.5f, hcol);
        }
      }
    }
#endif
  }

  // ===========================================================================
  // update
  // ===========================================================================
  void update() {
#ifndef GAME_BUILD
    if (!mode_enabled) return;

    // ---- Keyboard shortcuts (only when ImGui is NOT consuming keyboard) ----
    if (!ImGui::GetIO().WantCaptureKeyboard) {
      using K = Engine::KeyCode;
      if (Engine::Input::is_pressed_raw(K::P)) {
        pointer_tool_active = true;
      }
      else if (Engine::Input::is_pressed_raw(K::W)) {
        pointer_tool_active = false;
        ViewportToolbar::set_gizmo_mode(Components::GizmoMode::GrabMove);
        // Re-add Gizmo so the manipulation system picks the entity up
        Engine::ECS::Entity sel(selected_id);
        if (sel.is_valid() && sel.has_component<Components::Transform2D>() && !sel.has_component<Components::Gizmo>())
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(selected_id);
      }
      else if (Engine::Input::is_pressed_raw(K::E)) {
        pointer_tool_active = false;
        ViewportToolbar::set_gizmo_mode(Components::GizmoMode::ArrowMove);
        Engine::ECS::Entity sel(selected_id);
        if (sel.is_valid() && sel.has_component<Components::Transform2D>() && !sel.has_component<Components::Gizmo>())
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(selected_id);
      }
      else if (Engine::Input::is_pressed_raw(K::R)) {
        pointer_tool_active = false;
        ViewportToolbar::set_gizmo_mode(Components::GizmoMode::Rotate);
        Engine::ECS::Entity sel(selected_id);
        if (sel.is_valid() && sel.has_component<Components::Transform2D>() && !sel.has_component<Components::Gizmo>())
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(selected_id);
      }
      else if (Engine::Input::is_pressed_raw(K::T)) {
        pointer_tool_active = false;
        ViewportToolbar::set_gizmo_mode(Components::GizmoMode::Scale);
        Engine::ECS::Entity sel(selected_id);
        if (sel.is_valid() && sel.has_component<Components::Transform2D>() && !sel.has_component<Components::Gizmo>())
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(selected_id);
      }

      // Delete selected collider
      if (Engine::Input::is_pressed_raw(K::Delete)) {
        Engine::ECS::Entity sel(selected_id);
        if (sel.is_valid() && sel.has_component<Components::ManualCollider>()) {
          Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(selected_id, true);
          selected_id = Engine::ECS::INVALID_ENTITY;
          drag_vertex_idx = -1;
          drag_handle_idx = -1;
        }
      }

      // Ctrl+D: duplicate selected collider
      if (Engine::Input::is_pressed_raw(K::D) &&
        (Engine::Input::is_held_raw(K::CtrlLeft) ||
          Engine::Input::is_held_raw(K::CtrlRight))) {
        Engine::ECS::Entity sel(selected_id);
        if (sel.is_valid() && sel.has_component<Components::ManualCollider>()) {
          auto prefab = Engine::ECS::entity_to_prefab(sel);
          auto dup = Engine::ECS::instantiate_prefab(prefab);
          if (auto* dtr = dup.get_component<Components::Transform2D>())
            dtr->position.x += 64.0f;
          selected_id = dup.get_id();
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(selected_id);
          if (auto* dtr = dup.get_component<Components::Transform2D>())
            prev_scales[dup.get_id()] = dtr->scale;
          mark_dirty();
        }
      }

      // Escape: deselect
      if (Engine::Input::is_pressed_raw(K::Escape)) {
        selected_id = Engine::ECS::INVALID_ENTITY;
        Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(
          Engine::ECS::INVALID_ENTITY);
        drag_vertex_idx = -1;
        drag_handle_idx = -1;
      }
    }

    // ---- Sync selected_id from gizmo selection ----
    // If the entity with the Gizmo component is a ManualCollider, adopt it as our selection.
    Engine::ECS::for_each_entity_with<Components::Gizmo>([](Engine::ECS::Entity gizmo_entity) {
      if (gizmo_entity.has_component<Components::ManualCollider>()) {
        selected_id = gizmo_entity.get_id();
      }
    });

    // ---- Scale + Rotation sync for AABB/OBB ----
    // Runs every frame so the gizmo Scale/Rotate tools keep collider geometry in sync.
    for (auto& entity : Engine::ECS::get_entities_with<Components::ManualCollider>()) {
      auto* tr = entity.get_component<Components::Transform2D>();
      auto* col = entity.get_component<Components::BoxCollider>();
      if (!tr || !col) continue;

      // Force-sync: ensure tr->scale matches collider bounds so the gizmo system
      // always sees correct dimensions (fixes gizmo rendering for collider entities).
      force_sync_scale(entity, tr, col);

      // --- Rotation sync (OBB only) ---
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

      // --- Scale sync (AABB/OBB only) ---
      if (col->type == Components::ColliderType::POLYGON) continue;

      auto it = prev_scales.find(entity.get_id());
      if (it == prev_scales.end()) {
        prev_scales[entity.get_id()] = tr->scale;
        continue;
      }
      Math::Vec2D& prev = it->second;
      constexpr float eps = 0.001f;
      if (std::abs(prev.x - tr->scale.x) > eps || std::abs(prev.y - tr->scale.y) > eps)
        sync_collider_from_scale(entity, tr, col);
    }

    // ---- Pointer tool input ----
    if (!pointer_tool_active) return;
    if (ImGui::GetIO().WantCaptureMouse) return;

    auto mouse_opt = Engine::Input::get_mouse_world_position_if_in_viewport();
    if (!mouse_opt.has_value()) return;
    Math::Vec2D mouse = mouse_opt.value();

    bool m_pressed = Engine::Input::is_pressed(Engine::KeyCode::MouseLeft);
    bool m_held = Engine::Input::is_held(Engine::KeyCode::MouseLeft);
    bool m_released = Engine::Input::is_released(Engine::KeyCode::MouseLeft);
    bool r_pressed = Engine::Input::is_pressed(Engine::KeyCode::MouseRight);

    Engine::ECS::Entity sel(selected_id);
    if (!sel.is_valid() || !sel.has_component<Components::ManualCollider>()) {
      drag_vertex_idx = -1;
      drag_handle_idx = -1;
      return;
    }

    auto* tr = sel.get_component<Components::Transform2D>();
    auto* col = sel.get_component<Components::BoxCollider>();
    if (!tr || !col) return;

    // =========================================================================
    if (col->type == Components::ColliderType::POLYGON) {
      // ---- Polygon vertex editing ----

      if (m_released && drag_vertex_idx >= 0) {
        mark_dirty();
        drag_vertex_idx = -1;
      }

      if (m_held && drag_vertex_idx >= 0) {
        // Move vertex: convert world mouse to local space
        Math::Vec2D local = rotate(mouse - tr->position - col->offset, -tr->rotation);
        col->polygon.vertices[drag_vertex_idx] = local;
        return;
      }

      if (m_pressed) {
        auto wv = world_poly_verts(tr, col);
        float cr2 = CLICK_RADIUS * CLICK_RADIUS;

        // Check vertex handles first
        int best_v = -1; float best_d = cr2;
        for (int i = 0; i < static_cast<int>(wv.size()); ++i) {
          float d = dist2(mouse, wv[i]);
          if (d < best_d) { best_d = d; best_v = i; }
        }
        if (best_v >= 0) { drag_vertex_idx = best_v; return; }

        // Check edge midpoints for vertex insertion
        float er2 = (VERTEX_RADIUS * 0.55f + 4.0f);
        er2 = er2 * er2;
        int best_e = -1; float best_ed = er2;
        for (int i = 0; i < static_cast<int>(wv.size()); ++i) {
          int j = (i + 1) % static_cast<int>(wv.size());
          Math::Vec2D mid = { (wv[i].x + wv[j].x) * 0.5f,
                              (wv[i].y + wv[j].y) * 0.5f };
          float d = dist2(mouse, mid);
          if (d < best_ed) { best_ed = d; best_e = i; }
        }
        if (best_e >= 0) {
          // Insert new vertex at local-space midpoint
          int j = (best_e + 1) % static_cast<int>(wv.size());
          Math::Vec2D world_mid = { (wv[best_e].x + wv[j].x) * 0.5f,
                                    (wv[best_e].y + wv[j].y) * 0.5f };
          Math::Vec2D local_mid = rotate(world_mid - tr->position - col->offset, -tr->rotation);
          col->polygon.vertices.insert(
            col->polygon.vertices.begin() + best_e + 1, local_mid);
          drag_vertex_idx = best_e + 1;
          mark_dirty();
          return;
        }
      }

      // Right-click to delete vertex (minimum 3 vertices)
      if (r_pressed && col->polygon.vertices.size() > 3) {
        auto wv = world_poly_verts(tr, col);
        float cr2 = CLICK_RADIUS * CLICK_RADIUS;
        int best_v = -1; float best_d = cr2;
        for (int i = 0; i < static_cast<int>(wv.size()); ++i) {
          float d = dist2(mouse, wv[i]);
          if (d < best_d) { best_d = d; best_v = i; }
        }
        if (best_v >= 0) {
          col->polygon.vertices.erase(col->polygon.vertices.begin() + best_v);
          mark_dirty();
        }
      }

    }
    else {
      // =========================================================================
      // AABB / OBB resize handles
      // =========================================================================
      Math::Vec2D center = tr->position + col->offset;

      if (m_released && drag_handle_idx >= 0) {
        mark_dirty();
        drag_handle_idx = -1;
      }

      if (m_held && drag_handle_idx >= 0) {
        // Transform mouse into local space of the box
        Math::Vec2D local = rotate(mouse - center, -tr->rotation);
        constexpr float MIN_HALF = 2.0f;

        float new_hw = col->width * 0.5f;
        float new_hh = col->height * 0.5f;

        // Symmetric resize from center
        switch (drag_handle_idx) {
        case 0: new_hw = std::max(MIN_HALF, -local.x); break; // L
        case 1: new_hw = std::max(MIN_HALF, local.x); break; // R
        case 2: new_hh = std::max(MIN_HALF, local.y); break; // T
        case 3: new_hh = std::max(MIN_HALF, -local.y); break; // B
        case 4: new_hw = std::max(MIN_HALF, -local.x);        // TL
          new_hh = std::max(MIN_HALF, local.y); break;
        case 5: new_hw = std::max(MIN_HALF, local.x);        // TR
          new_hh = std::max(MIN_HALF, local.y); break;
        case 6: new_hw = std::max(MIN_HALF, -local.x);        // BL
          new_hh = std::max(MIN_HALF, -local.y); break;
        case 7: new_hw = std::max(MIN_HALF, local.x);        // BR
          new_hh = std::max(MIN_HALF, -local.y); break;
        default: break;
        }

        col->width = new_hw * 2.0f;
        col->height = new_hh * 2.0f;
        col->aabb.half_width = new_hw;
        col->aabb.half_height = new_hh;
        if (col->type == Components::ColliderType::OBB) {
          col->polygon = Polygon::create_box(col->width, col->height);
          col->polygon.rotation = tr->rotation;
        }
        tr->scale = { col->width, col->height };
        prev_scales[sel.get_id()] = tr->scale; // suppress double-sync
        return;
      }

      if (m_pressed) {
        Math::Vec2D handles[8];
        compute_box_handles(tr, col, handles);
        float cr2 = CLICK_RADIUS * CLICK_RADIUS;
        int best_h = -1; float best_d = cr2;
        for (int i = 0; i < 8; ++i) {
          float d = dist2(mouse, handles[i]);
          if (d < best_d) { best_d = d; best_h = i; }
        }
        drag_handle_idx = best_h; // -1 if nothing was hit
      }
    }
#endif
  }

  // ===========================================================================
  // Public accessors
  // ===========================================================================
  bool is_manual_collider_mode_enabled() { return mode_enabled; }
  bool is_pointer_tool_active() { return mode_enabled && pointer_tool_active; }
  void set_pointer_tool_active(bool active) { if (mode_enabled) pointer_tool_active = active; }

} // namespace ManualColliderPanel
