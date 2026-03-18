//-----------------------------------------------------------------------------------
// file:    ManualColliderPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Editor panel for manually placing and editing AABB/OBB collider entities
//          in the world. Colliders behave like normal entities and are selectable
//          and manipulable with the standard gizmo tools.
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
  // Draws world-space collider outlines.
  // Call inside the world-space drawing batch (after begin_batch, before end_batch),
  // at the same point TilemapPanel::draw_overlay() is called.
  //=================================================================================
  void draw_overlay();

  //=================================================================================
  // Handles per-frame logic: Transform2D.scale -> BoxCollider sync, AABB rotation
  // lock. Call during the update phase, alongside TilemapPanel::update().
  //=================================================================================
  void update();

} // namespace ManualColliderPanel
