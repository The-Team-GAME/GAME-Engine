//-----------------------------------------------------------------------------------
// file:    ManualColliderPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Editor panel for manually placing and editing collider entities in the
//          world (AABB, OBB, Polygon). Operates as a distinct "Manual Collider Mode"
//          alongside Tilemap Mode. Provides a Pointer tool for direct vertex and
//          resize-handle editing in the viewport, and reuses the standard gizmo for
//          move / rotate / scale operations.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

namespace ManualColliderPanel {

  //=================================================================================
  // Renders the ImGui panel (collider list, creation buttons, property editor).
  // Call during the ImGui phase.
  //=================================================================================
  void draw_panel();

  //=================================================================================
  // Draws world-space collider outlines and vertex/handle overlays.
  // Call inside the world-space drawing batch (after begin_batch, before end_batch),
  // at the same point TilemapPanel::draw_overlay() is called.
  //=================================================================================
  void draw_overlay();

  //=================================================================================
  // Handles per-frame input: keyboard shortcuts, Pointer tool vertex/handle dragging,
  // and Transform2D.scale -> BoxCollider sync.
  // Call during the update phase, alongside TilemapPanel::update().
  //=================================================================================
  void update();

  //=================================================================================
  // Returns true when Manual Collider Mode is active.
  // Used by ViewportToolbar to show the collider tool buttons instead of the
  // standard gizmo buttons.
  //=================================================================================
  bool is_manual_collider_mode_enabled();

  //=================================================================================
  // Returns true when the Pointer tool is the active sub-tool within Manual Collider
  // Mode.  GizmoManipulationSystem checks this to suppress gizmo drag on all
  // entities while the Pointer tool is handling vertex / resize-handle interaction.
  //=================================================================================
  bool is_pointer_tool_active();

  //=================================================================================
  // Activates or deactivates the Pointer sub-tool from outside (e.g. toolbar button).
  //=================================================================================
  void set_pointer_tool_active(bool active);

} // namespace ManualColliderPanel
