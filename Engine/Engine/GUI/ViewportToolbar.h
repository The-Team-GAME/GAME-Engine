//-----------------------------------------------------------------------------------
// file:    ViewportToolbar.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Extensible viewport toolbar overlay that renders tool selection buttons
//          on the left edge of the viewport. Currently hosts gizmo tool selection
//          (Grab, Arrow Move, Rotate, Scale) with Lucide icons. Designed for
//          extensibility — additional tool sections (e.g., tilemap tools) can be
//          registered via register_tool_section().
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Components/GizmoComponents.h"
#include <functional>
#include <string>

namespace ViewportToolbar {

  //=================================================================================
  // Initialization — loads the Lucide icon font into ImGui's font atlas.
  // Must be called after ImGui::CreateContext() and before the first NewFrame().
  //=================================================================================
  void init();

  //=================================================================================
  // Draws the toolbar overlay. Call this inside the Viewport ImGui window
  // (after ImGui::Image, before ImGui::End) so the overlay is part of the
  // viewport panel.
  //=================================================================================
  void draw();

  //=================================================================================
  // Gizmo tool mode — global state (replaces per-entity Gizmo::mode).
  // GizmoManipulationSystem reads this to decide which handles are active.
  //=================================================================================
  Components::GizmoMode get_gizmo_mode();
  void set_gizmo_mode(Components::GizmoMode mode);

  //=================================================================================
  // Extensibility — register additional tool sections that appear below the
  // built-in gizmo tools. Each section has a label and a draw callback.
  // The callback should draw ImGui widgets (buttons, etc.) assuming the
  // cursor is already positioned inside the toolbar column.
  //
  // Example (future tilemap integration):
  //   ViewportToolbar::register_tool_section("Tilemap", []() {
  //     if (icon_button(ICON_PAINTBRUSH, ...)) { ... }
  //   });
  //=================================================================================
  using ToolSectionCallback = std::function<void()>;
  void register_tool_section(const std::string& name, ToolSectionCallback callback);

  //=================================================================================
  // Returns the loaded Lucide icon ImFont*, or nullptr if not yet initialized.
  // Useful if other code wants to push this font temporarily.
  // (Returns void* to avoid pulling imgui.h into this header; cast to ImFont*.)
  //=================================================================================
  void* get_icon_font();

} // namespace ViewportToolbar
