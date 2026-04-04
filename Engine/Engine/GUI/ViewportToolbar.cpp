//-----------------------------------------------------------------------------------
// file:    ViewportToolbar.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of the extensible viewport toolbar overlay. Loads the
//          Lucide icon font, renders a vertical icon bar on the left edge of the
//          viewport, and manages the global gizmo tool mode. Supports registering
//          additional tool sections for future extensibility.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "../Precompiled.h"
#include "ViewportToolbar.h"
#include "LucideIcons.h"
#include "ViewportPanel.h"
#include "TilemapPanel.h"
#include "ManualColliderPanel.h"
#include "../ImGuiManager.h"
#include "../Logger.h"
#include "../InputManager.h"


namespace ViewportToolbar {

  // ---------------------------------------------------------------------------
  // Internal state
  // ---------------------------------------------------------------------------
  namespace {
    ::ImFont* icon_font = nullptr;
    Components::GizmoMode current_gizmo_mode = Components::GizmoMode::GrabMove;

    struct ToolSection {
      std::string name;
      ToolSectionCallback callback;
    };
    std::vector<ToolSection> registered_sections;

    // Toolbar styling constants
    constexpr float BUTTON_SIZE = 28.0f;     // Icon button size (square)
    constexpr float TOOLBAR_PADDING = 4.0f;  // Padding inside the toolbar
    constexpr float TOOLBAR_OFFSET_X = 6.0f; // Offset from left edge of viewport
    constexpr float TOOLBAR_OFFSET_Y = 6.0f; // Offset from top edge of viewport
    constexpr float BUTTON_SPACING = 2.0f;   // Spacing between buttons

    // Helper: draw an icon button with selection highlight
    bool icon_button(const char* icon, const char* tooltip, bool is_selected) {
      ImVec4 bg_color = is_selected
        ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)
        : ImVec4(0, 0, 0, 0);

      ImGui::PushStyleColor(ImGuiCol_Button, bg_color);

      bool clicked = false;
      if (icon_font) {
        ImGui::PushFont(icon_font);
        clicked = ImGui::Button(icon, ImVec2(BUTTON_SIZE, BUTTON_SIZE));
        ImGui::PopFont();
      }
      else {
        // Fallback: use first character of tooltip as label
        char fallback[2] = { tooltip[0], 0 };
        clicked = ImGui::Button(fallback, ImVec2(BUTTON_SIZE, BUTTON_SIZE));
      }

      ImGui::PopStyleColor();

      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
      }

      return clicked;
    }
  }

  // ---------------------------------------------------------------------------
  // Public API
  // ---------------------------------------------------------------------------

  void init() {
    // Load Lucide icon font into ImGui's font atlas.
    // This must be called after ImGui::CreateContext() and before the first NewFrame()
    // (ImGui builds the font atlas lazily on the first frame).
    ImGuiIO& io = ImGui::GetIO();

    // Build the path to the font file.
    // This runs during ImGuiManager::init(), before Resources::init(), so
    // resource_path is not yet available. Use a relative path from the working
    // directory (which is the Engine/ folder at startup).
    std::string font_path = "Assets/fonts/lucide.ttf";

    // Ensure the default font is added first so other parts of the engine
    // still have text. ImGui only auto-adds the default font if no fonts
    // were added before the atlas is built.
    io.Fonts->AddFontDefault();

    // Load as a separate font (not merged) at a size that looks good for toolbar icons
    float icon_size = 18.0f;

    // Define the glyph range to load (only the icons we need)
    static const ImWchar icon_ranges[] = {
      LUCIDE_ICON_MIN, LUCIDE_ICON_MAX,
      0 // Null terminator
    };

    icon_font = io.Fonts->AddFontFromFileTTF(font_path.c_str(), icon_size, nullptr, icon_ranges);

    if (!icon_font) {
      Log::warning("ViewportToolbar: Failed to load Lucide icon font from '{}'", font_path);
    }
    else {
      Log::info("ViewportToolbar: Loaded Lucide icon font from '{}'", font_path);
    }
  }

  void draw() {
#ifndef GAME_BUILD
    // Get viewport image bounds to position the toolbar
    Math::Vec4D bounds_vec = ViewportPanel::get_viewport_bounds();
    ImVec2 vp_min = { bounds_vec.x, bounds_vec.y };
    ImVec2 vp_max = { bounds_vec.z, bounds_vec.w };

    // Don't draw if viewport has no area
    if (vp_max.x - vp_min.x < 1.0f || vp_max.y - vp_min.y < 1.0f)
      return;

    // Position the toolbar on the left edge of the viewport image
    ImVec2 toolbar_pos = {
      vp_min.x + TOOLBAR_OFFSET_X,
      vp_min.y + TOOLBAR_OFFSET_Y
    };

    ImGui::SetNextWindowPos(toolbar_pos);
    ImGui::SetNextWindowBgAlpha(0.85f);

    ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoDocking |
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoFocusOnAppearing;

    // ---- Gizmo keybinds (outside ImGui window, raw input like TilemapPanel) ----
    if (!TilemapPanel::is_tilemap_mode_enabled()) {
      if (Engine::Input::is_pressed_raw(Engine::KeyCode::W)) current_gizmo_mode = Components::GizmoMode::GrabMove;
      else if (Engine::Input::is_pressed_raw(Engine::KeyCode::E)) current_gizmo_mode = Components::GizmoMode::ArrowMove;
      else if (Engine::Input::is_pressed_raw(Engine::KeyCode::R)) current_gizmo_mode = Components::GizmoMode::Rotate;
      else if (Engine::Input::is_pressed_raw(Engine::KeyCode::T)) current_gizmo_mode = Components::GizmoMode::Scale;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(TOOLBAR_PADDING, TOOLBAR_PADDING));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(BUTTON_SPACING, BUTTON_SPACING));

    if (ImGui::Begin("##ViewportToolbar", nullptr, flags)) {
      if (TilemapPanel::is_tilemap_mode_enabled()) {
        // ---- Tilemap tool buttons ----
        auto tool = TilemapPanel::get_tool_mode();

        if (icon_button(LucideIcons::Brush, "Paintbrush (B)", tool == TilemapPanel::ToolMode::Paintbrush))
          TilemapPanel::set_tool_mode(TilemapPanel::ToolMode::Paintbrush);

        if (icon_button(LucideIcons::PaintRoller, "Box Fill (U)", tool == TilemapPanel::ToolMode::BoxFill))
          TilemapPanel::set_tool_mode(TilemapPanel::ToolMode::BoxFill);

        if (icon_button(LucideIcons::Pipette, "Picker (I)", tool == TilemapPanel::ToolMode::Picker))
          TilemapPanel::set_tool_mode(TilemapPanel::ToolMode::Picker);

        if (icon_button(LucideIcons::Eraser, "Eraser (D)", tool == TilemapPanel::ToolMode::Eraser))
          TilemapPanel::set_tool_mode(TilemapPanel::ToolMode::Eraser);

        if (icon_button(LucideIcons::BoxSelect, "Select (M)", tool == TilemapPanel::ToolMode::Select))
          TilemapPanel::set_tool_mode(TilemapPanel::ToolMode::Select);
      }
      else {
        // ---- Gizmo tool buttons (vertical layout) ----
        if (icon_button(LucideIcons::Move, "Grab / Move (W)", current_gizmo_mode == Components::GizmoMode::GrabMove))
          current_gizmo_mode = Components::GizmoMode::GrabMove;

        if (icon_button(LucideIcons::Crosshair, "Arrows X/Y (E)", current_gizmo_mode == Components::GizmoMode::ArrowMove))
          current_gizmo_mode = Components::GizmoMode::ArrowMove;

        if (icon_button(LucideIcons::RotateCw, "Rotate (R)", current_gizmo_mode == Components::GizmoMode::Rotate))
          current_gizmo_mode = Components::GizmoMode::Rotate;

        if (icon_button(LucideIcons::Scaling, "Scale (T)", current_gizmo_mode == Components::GizmoMode::Scale))
          current_gizmo_mode = Components::GizmoMode::Scale;
      }

      // ---- Registered tool sections ----
      if (!registered_sections.empty()) {
        ImGui::Separator();
        for (auto& section : registered_sections) {
          section.callback();
        }
      }
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
#endif
  }

  Components::GizmoMode get_gizmo_mode() {
    return current_gizmo_mode;
  }

  void set_gizmo_mode(Components::GizmoMode mode) {
    current_gizmo_mode = mode;
  }

  void register_tool_section(const std::string& name, ToolSectionCallback callback) {
    registered_sections.push_back({ name, std::move(callback) });
  }

  void* get_icon_font() {
    return icon_font;
  }

} // namespace ViewportToolbar

