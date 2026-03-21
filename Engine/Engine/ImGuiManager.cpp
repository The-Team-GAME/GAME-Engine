//-----------------------------------------------------------------------------------
// file:    ImGuiManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the ImGuiManager which manages ImGui context,
//          frame rendering, and provides toggle functionality for showing/hiding
//          the debug UI with the F1 key.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ImGuiManager.h"
#include "WindowManager.h"
#include "Precompiled.h"
#include <filesystem>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include "InputManager.h"
#include "Perf.h"
#include "ImGuiThemes.h"
#include "FileIO/Settings.h"
#include "Core.h"
#include "SceneManager.h"
#include "ProjectSaveManager.h"
#include "Events/WindowEvents.h"
#include "EventManager/EventManager.h"
#include "GUI/ViewportPanel.h"
#include "GUI/ViewportToolbar.h"
#include "HotReloadManager.h"
#include "Graphics/Renderer.h"

namespace {
  bool gui_enabled = true; // Completely disable ImGui when false (TODO: Hook this up to a setting)
  bool main_gui_visible = true; // Still render ImGui but hide all windows when false
  bool show_exit_confirmation = false;
  std::vector<char> exit_save_selections; // Tracks which dirty items user wants to save on exit (char instead of bool to avoid vector<bool> proxy)

  bool should_imgui_yield_keyboard_to_game()
  {
    if (!gui_enabled || !main_gui_visible)
      return false;

    if (!Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().panel_viewport_mode)
      return false;

    return ViewportPanel::is_viewport_focused() || ViewportPanel::is_viewport_hovered();
  }

  struct PanelInfo {
    std::string name;
    std::function<void()> render_func;
    bool visible = true;
  };

  struct Toast {
    std::string message;
    ImGuiManager::ToastType type;
    float duration;
    float elapsed;
  };

  std::vector<PanelInfo> registered_panels;
  std::vector<Toast> active_toasts;
  ImGuiID main_dockspace_id = 0;

  // ---- Layout state ----
  constexpr const char* LAYOUTS_DIR = "Assets/layouts/";
  std::string current_layout_name = "default";
  bool show_save_layout_popup = false;
  char save_layout_name_buf[128] = {};
  // ---- End Layout state ----

  std::unordered_map<std::string, std::function<void()>> styles_map = {
    {"Eggplant", SetupImGuiStyle_Eggplant},
    {"Moonlight", SetupImGuiStyle_Moonlight},
    {"Photoshop", SetupImGuiStyle_Photoshop},
    {"Windark", SetupImGuiStyle_Windark},
    {"ayu-dark", SetupImGuiStyle_ayu_dark},
    {"Enemymouse", SetupImGuiStyle_Enemymouse},
    {"Vullmad Dark", SetupImGuiStyle_Vullmad_Dark},
    {"Comfortable Dark Cyan", SetupImGuiStyle_Comfortable_Dark_Cyan}
  };
}

namespace ImGuiManager {

  void init()
  {
#ifndef GAME_BUILD
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // Disable automatic imgui.ini save/load; we manage layouts manually
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      style.WindowRounding = 0.0f;
      style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui::StyleColorsDark();

    // Load icon fonts before backend init (atlas must be built before first frame)
    ViewportToolbar::init();

    ImGui_ImplGlfw_InitForOpenGL(Engine::WindowManager::get_window(), true);
    ImGui_ImplOpenGL3_Init();

    // Register event listener for window close requests
    Engine::EventManager::add_event_listener<Engine::Events::WindowCloseRequestedEvent>(
      [](const Engine::Events::WindowCloseRequestedEvent&) {
        show_exit_confirmation = true;
      }
    );

    // Register event listener for toast notifications
    Engine::EventManager::add_event_listener<Engine::Events::ToastEvent>(
      [](const Engine::Events::ToastEvent& event) {
        show_toast(event.message, event.type, event.duration);
      }
    );

    // Auto-load last used layout (or seed a default if none exists yet)
    {
      const std::string& saved_layout = Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::get().imgui_layout;
      current_layout_name = saved_layout.empty() ? "default" : saved_layout;
      std::string layout_path = std::string(LAYOUTS_DIR) + current_layout_name + ".ini";
      if (std::filesystem::exists(layout_path))
      {
        ImGui::LoadIniSettingsFromDisk(layout_path.c_str());
      }
      // (If no file exists yet the first explicit save will create it)
    }
#endif
  }

  void exit()
  {
#ifndef GAME_BUILD
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
#endif
  }

  void begin()
  {
#ifndef GAME_BUILD
    if (!gui_enabled) {
      return;
    }

    // ==================================================================================
    // Always start ImGui frame (needed for popups even when GUI is disabled)
    // ==================================================================================
    ImGuiIO& io = ImGui::GetIO();
    if (should_imgui_yield_keyboard_to_game())
    {
      io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
    }
    else
    {
      io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (should_imgui_yield_keyboard_to_game())
    {
      io.WantCaptureKeyboard = false;
    }

    // ==================================================================================
    // Start of ImGui enabled
    // ==================================================================================
    if (main_gui_visible)
    {

      ImGuiViewport* viewport = ImGui::GetMainViewport();
      float top_bar_height = 0.0f;

      // 1. Menu Bar and Toolbar Dockspace (Combined)
      {
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, 0)); // Auto height
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        window_flags |= ImGuiWindowFlags_NoScrollbar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("##MenuBarAndToolbarDockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        // Add a menu bar
        if (ImGui::BeginMenuBar())
        {
          if (ImGui::BeginMenu("File"))
          {
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
              Engine::ProjectSaveManager::save_all_dirty();
            }
            ImGui::EndMenu();
          }
          if (ImGui::BeginMenu("ImGui"))
          {
            ImGui::MenuItem("Toggle GUI", "F1", &main_gui_visible);
            ImGui::MenuItem("Panel Viewport Mode", "F2", &Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get_mutable().panel_viewport_mode);
            if (ImGui::MenuItem("Toggle Debug Layer", "F3", Engine::ECS::is_layer_active("debug"))) {
              Engine::ECS::is_layer_active("debug") ? Engine::ECS::disable_layer("debug") : Engine::ECS::enable_layer("debug");
            }
            if (ImGui::MenuItem("Toggle Fullscreen", "F11", &Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get_mutable().fullscreen)) {
              Engine::WindowManager::set_fullscreen(Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get().fullscreen);
              Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::save();
            }
            if (ImGui::MenuItem("Toggle VSync", nullptr, &Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get_mutable().vsync)) {
              Engine::WindowManager::set_vsync(Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get().vsync);
              Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::save();
            }

            ImGui::EndMenu();
          }
          if (ImGui::BeginMenu("Windows"))
          {
            for (auto& panel : registered_panels)
            {
              ImGui::MenuItem(panel.name.c_str(), nullptr, &panel.visible);
            }
            ImGui::EndMenu();
          }
          if (ImGui::BeginMenu("Styles"))
          {
            for (const auto& [style_name, _] : styles_map)
            {
              if (ImGui::MenuItem(style_name.c_str()))
              {
                set_style(style_name);
              }
            }
            ImGui::EndMenu();
          }
          if (ImGui::BeginMenu("Layouts"))
          {
            if (ImGui::MenuItem("Save Current Layout"))
            {
              save_layout(current_layout_name);
              show_toast("Layout '" + current_layout_name + "' saved", ImGuiManager::ToastType::Info, 2.0f);
            }
            if (ImGui::MenuItem("Save Layout As..."))
            {
              // Pre-fill with current name
              strncpy_s(save_layout_name_buf, sizeof(save_layout_name_buf), current_layout_name.c_str(), sizeof(save_layout_name_buf) - 1);
              show_save_layout_popup = true;
            }
            ImGui::Separator();
            auto layouts = get_available_layouts();
            if (layouts.empty())
            {
              ImGui::TextDisabled("(no saved layouts)");
            }
            for (const auto& layout_name : layouts)
            {
              bool is_active = (layout_name == current_layout_name);
              if (ImGui::MenuItem(layout_name.c_str(), nullptr, is_active))
              {
                load_layout(layout_name);
                Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::get_mutable().imgui_layout = layout_name;
                Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::save();
              }
            }
            ImGui::EndMenu();
          }
          ImGui::EndMenuBar();
        }

        // Toolbar content below the menu bar
        // Apply a slightly darker background than the menu bar
        ImVec4 menubar_color = ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg);
        ImVec4 toolbar_color = ImVec4(
          menubar_color.x * 0.9f,  // 20% darker
          menubar_color.y * 0.9f,
          menubar_color.z * 0.9f,
          menubar_color.w
        );
        ImGui::PushStyleColor(ImGuiCol_ChildBg, toolbar_color);
        ImGui::BeginChild("##Toolbar", ImVec2(0, 40), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::Dummy(ImVec2(0.0f, 2.0f)); // Top padding

        ImGui::Indent(8.0f); // Left padding
        bool is_paused = Engine::Time::get_paused();


        // Play/Pause toggle button
        if (is_paused) {
          if (ImGui::Button("Play", ImVec2(80, 0))) {
            Engine::Time::set_paused(false);
          }
        }
        else {
          if (ImGui::Button("Pause", ImVec2(80, 0))) {
            Engine::Time::set_paused(true);
          }
        }

        ImGui::SameLine();

        // Stop button 
        if (ImGui::Button("Stop", ImVec2(60, 0))) {
          std::string current = Engine::SceneManager::get_current_scene_name();
          Engine::SceneManager::unload_current_scene();
          Engine::SceneManager::load_scene(current);
          Engine::Time::set_paused(true);
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Build Button
        if (Engine::HotReload::is_building())
        {
          if (ImGui::Button("Cancel", ImVec2(80, 0)))
          {
            Engine::HotReload::request_build_cancellation();
          }
        }
        else
        {
          if (ImGui::Button("Build", ImVec2(80, 0)))
          {
            Engine::HotReload::request_manual_reload();
          }
        }

        ImGui::Unindent(8.0f); // Remove left padding
        ImGui::Dummy(ImVec2(0.0f, 4.0f)); // Bottom padding

        ImGui::EndChild();
        ImGui::PopStyleColor();

        top_bar_height = ImGui::GetWindowHeight();
        ImGui::End();
      }

      // 2. Main Panels Dockspace
      {
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + top_bar_height));
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - top_bar_height));
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("##PanelsDockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        // DockSpace for all panels
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        main_dockspace_id = dockspace_id;
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::End();
      }

      // Render all registered panels
      for (const auto& panel : registered_panels)
      {
        if (panel.visible)
        {
          panel.render_func();
        }
      }
    }

    // ==================================================================================
    // End of ImGui enabled
    // ==================================================================================


    // ==================================================================================
    // Start of run even when ImGui is disabled
    // ==================================================================================

    // Show exit confirmation popup
    if (show_exit_confirmation)
    {
      ImGui::OpenPopup("Exit Confirmation");
      show_exit_confirmation = false; // Reset flag after opening
    }

    // Show "Save Layout As..." popup
    if (show_save_layout_popup)
    {
      ImGui::OpenPopup("Save Layout As");
      show_save_layout_popup = false;
    }
    {
      ImVec2 center = ImGui::GetMainViewport()->GetCenter();
      ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
      ImGui::SetNextWindowSize(ImVec2(340, 0), ImGuiCond_Appearing);
      if (ImGui::BeginPopupModal("Save Layout As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
      {
        ImGui::Text("Layout name:");
        ImGui::SetNextItemWidth(-1.0f);
        bool confirm = ImGui::InputText("##layout_name_input", save_layout_name_buf, sizeof(save_layout_name_buf),
          ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        f32 btn_w = 120.0f;
        f32 spacing = ImGui::GetStyle().ItemSpacing.x;
        f32 total = btn_w * 2 + spacing;
        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x - total) * 0.5f + ImGui::GetWindowContentRegionMin().x);

        if (ImGui::Button("Save", ImVec2(btn_w, 0)) || confirm)
        {
          std::string name = save_layout_name_buf;
          // Trim whitespace
          name.erase(0, name.find_first_not_of(" \t"));
          name.erase(name.find_last_not_of(" \t") + 1);
          if (!name.empty())
          {
            save_layout(name);
            Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::get_mutable().imgui_layout = name;
            Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::save();
            show_toast("Layout '" + name + "' saved", ImGuiManager::ToastType::Info, 2.0f);
          }
          ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(btn_w, 0)))
        {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }
    }

    // Always place the popup at the center of the viewport
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(350, 0), ImVec2(500, 600));

    if (ImGui::BeginPopupModal("Exit Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      const auto& dirty_states = Engine::ProjectSaveManager::get_dirty_states();
      bool has_unsaved_changes = !dirty_states.empty();

      if (has_unsaved_changes)
      {
        ImGui::Text("You have unsaved changes!");
        ImGui::Spacing();
        ImGui::Text("Select which items to save before exiting:");
        ImGui::Spacing();

        // Track which items the user wants to save (persistent across frames)
        if (exit_save_selections.size() != dirty_states.size()) {
          exit_save_selections.assign(dirty_states.size(), true); // Default: save all
        }

        // Show each dirty item with a checkbox
        ImGui::BeginChild("DirtyItems", ImVec2(0, std::min(200.0f, dirty_states.size() * 28.0f + 8.0f)), true);
        for (size_t i = 0; i < dirty_states.size(); ++i) {
          std::string display_name = Engine::ProjectSaveManager::get_dirty_display_name(dirty_states[i]);
          std::string label = display_name + "##dirty_" + std::to_string(i);
          bool selected = exit_save_selections[i] != 0;
          if (ImGui::Checkbox(label.c_str(), &selected)) {
            exit_save_selections[i] = selected ? 1 : 0;
          }
        }
        ImGui::EndChild();

        ImGui::Spacing();

        // Select All / Deselect All
        if (ImGui::SmallButton("Select All")) {
          std::fill(exit_save_selections.begin(), exit_save_selections.end(), true);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Deselect All")) {
          std::fill(exit_save_selections.begin(), exit_save_selections.end(), false);
        }
      }
      else
      {
        ImGui::Text("Are you sure you want to exit?");
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Button layout
      f32 button_width = 120.0f;
      f32 total_width = has_unsaved_changes ? (button_width * 3 + ImGui::GetStyle().ItemSpacing.x * 2)
        : (button_width * 2 + ImGui::GetStyle().ItemSpacing.x);
      f32 window_width = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
      f32 offset = (window_width - total_width) * 0.5f;

      if (offset > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

      if (has_unsaved_changes)
      {
        // Save Selected button — saves checked items and exits
        if (ImGui::Button("Save Selected", ImVec2(button_width, 0)))
        {
          // Save selected items in reverse order so indices stay valid
          for (int i = static_cast<int>(exit_save_selections.size()) - 1; i >= 0; --i) {
            if (exit_save_selections[static_cast<size_t>(i)]) {
              Engine::ProjectSaveManager::save_single_dirty(static_cast<size_t>(i));
            }
          }
          exit_save_selections.clear();
          Engine::Core::stop();
          ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        // Don't Save button — exits without saving
        if (ImGui::Button("Don't Save", ImVec2(button_width, 0)))
        {
          exit_save_selections.clear();
          Engine::Core::stop();
          ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
      }
      else
      {
        // Exit button
        if (ImGui::Button("Exit", ImVec2(button_width, 0)))
        {
          Engine::Core::stop();
          ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
      }

      // Cancel button
      if (ImGui::Button("Cancel", ImVec2(button_width, 0)))
      {
        exit_save_selections.clear();
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    // ==================================================================================
    // End of run even when ImGui is disabled
    // ==================================================================================

    // ==================================================================================
    // "Building..." overlay (top-right, persistent while build is running)
    // ==================================================================================
    {
      static bool was_building = false;
      bool currently_building = Engine::HotReload::is_building();

      // Detect build cancellation → show warning toast
      if (Engine::HotReload::consume_build_cancelled())
      {
        show_toast("Build cancelled",
          ImGuiManager::ToastType::Warning, 3.0f);
      }

      // Detect build completion → show toast with total time
      if (was_building && !currently_building)
      {
        float duration = Engine::HotReload::get_last_build_duration();
        bool success = Engine::HotReload::last_build_succeeded();
        int errors = Engine::HotReload::get_build_error_count();
        char buf[128];
        if (success)
          std::snprintf(buf, sizeof(buf), "Build succeeded in %.1fs", duration);
        else
          std::snprintf(buf, sizeof(buf), "Build FAILED after %.1fs (%d error%s)",
            duration, errors, errors == 1 ? "" : "s");

        show_toast(buf,
          success ? ImGuiManager::ToastType::Info : ImGuiManager::ToastType::Error,
          5.0f);
      }
      was_building = currently_building;

      if (currently_building)
      {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos;
        ImVec2 work_size = viewport->WorkSize;

        float padding = 16.0f;
        float toast_width = 260.0f;

        ImVec2 build_pos = ImVec2(
          work_pos.x + work_size.x - toast_width - padding,
          work_pos.y + padding
        );

        ImGui::SetNextWindowPos(build_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(toast_width, 0), ImGuiCond_Always);

        ImVec4 accent(0.1f, 0.7f, 0.9f, 1.0f); // teal / cyan accent

        ImGui::PushStyleColor(ImGuiCol_Border, accent);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.15f, 0.95f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));

        ImGuiWindowFlags build_flags =
          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
          ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
          ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs |
          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        if (ImGui::Begin("##BuildingOverlay", nullptr, build_flags))
        {
          // Animated spinner dots
          const char* dots[] = { "   ", ".  ", ".. ", "..." };
          int dot_idx = static_cast<int>(ImGui::GetTime() * 3.0f) % 4;

          float elapsed = Engine::HotReload::get_build_elapsed_seconds();

          ImGui::TextColored(accent, "[BUILD]");
          ImGui::SameLine();
          ImGui::Text("Building GameLogic%s", dots[dot_idx]);
          ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Elapsed: %.1fs", elapsed);

          // Show current file being compiled
          std::string current_file = Engine::HotReload::get_current_build_status();
          if (!current_file.empty())
          {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", current_file.c_str());
          }
        }
        ImGui::End();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(2);
      }
    }

    // ==================================================================================
    // Render toast notifications
    // ==================================================================================
    if (!active_toasts.empty())
    {
      ImGuiViewport* viewport = ImGui::GetMainViewport();
      ImVec2 work_pos = viewport->WorkPos;
      ImVec2 work_size = viewport->WorkSize;

      float padding = 10.0f;
      float toast_width = 300.0f;
      float toast_spacing = 10.0f;
      float toast_height = 60.0f; // Approximate height per toast
      float y_offset = work_size.y - padding;

      for (size_t i = 0; i < active_toasts.size(); ++i)
      {
        Toast& toast = active_toasts[i];

        // Calculate position (bottom-right corner, stacking upwards)
        y_offset -= toast_height;
        ImVec2 toast_pos = ImVec2(
          work_pos.x + work_size.x - toast_width - padding,
          work_pos.y + y_offset
        );

        ImGui::SetNextWindowPos(toast_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(toast_width, 0), ImGuiCond_Always);

        // Style based on toast type
        ImVec4 border_color;
        const char* icon;
        switch (toast.type)
        {
        case ImGuiManager::ToastType::Info:
          border_color = ImVec4(0.2f, 0.4f, 0.8f, 1.0f);
          icon = "[INFO]";
          break;
        case ImGuiManager::ToastType::Warning:
          border_color = ImVec4(0.8f, 0.6f, 0.2f, 1.0f);
          icon = "[WARNING]";
          break;
        case ImGuiManager::ToastType::Error:
          border_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
          icon = "[ERROR]";
          break;
        default:
          border_color = ImVec4(0.2f, 0.4f, 0.8f, 1.0f);
          icon = "[INFO]";
          break;
        }

        ImGui::PushStyleColor(ImGuiCol_Border, border_color);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);

        std::string window_id = "##Toast" + std::to_string(i);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
          ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
          ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs |
          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;        if (ImGui::Begin(window_id.c_str(), nullptr, flags))
        {
          ImGui::TextColored(border_color, "%s", icon);
          ImGui::SameLine();
          ImGui::TextWrapped("%s", toast.message.c_str());

          // Progress bar with matching color
          ImGui::PushStyleColor(ImGuiCol_PlotHistogram, border_color);
          float progress = toast.elapsed / toast.duration;
          ImGui::ProgressBar(progress, ImVec2(-1.0f, 2.0f), "");
          ImGui::PopStyleColor();
        }
        ImGui::End();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        // Update y_offset for next toast (subtract spacing for upward stacking)
        y_offset -= toast_spacing;
      }
    }

    // ==================================================================================
    // End of toast notifications
    // ==================================================================================
#endif
  }

  void end()
  {
#ifndef GAME_BUILD
    Engine::Perf::start("ImGui Render");

    if (!gui_enabled)
    {
      return;
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      GLFWwindow* backup_current_context = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup_current_context);
    }

    Engine::Perf::stop("ImGui Render");
#endif
  }

  void update()
  {
#ifndef GAME_BUILD
    // Update toast timers
    float dt = Engine::Time::get_unscaled_fixed_dt();
    for (auto it = active_toasts.begin(); it != active_toasts.end();)
    {
      it->elapsed += dt;
      if (it->elapsed >= it->duration)
      {
        it = active_toasts.erase(it);
      }
      else
      {
        ++it;
      }
    }

    // Save
    if (Engine::Input::is_held_raw(Engine::KeyCode::CtrlLeft) && Engine::Input::is_pressed_raw(Engine::KeyCode::S))
    {
      Engine::ProjectSaveManager::save_all_dirty();
    }

    // Toggle ImGui with F1 key
    if (Engine::Input::is_pressed_raw(Engine::KeyCode::F1))
    {
      main_gui_visible = !main_gui_visible;
    }

    // Toggle Panel Viewport Mode with F2 key
    if (Engine::Input::is_pressed_raw(Engine::KeyCode::F2))
    {
      Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get_mutable().panel_viewport_mode =
        !Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().panel_viewport_mode;
    }

    // Make sure that panel_viewport_mode is false when gui is disabled
    if (!main_gui_visible) Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get_mutable().panel_viewport_mode = false;

    // Clear screen every frame when panel_viewport_mode is true
    if (Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().panel_viewport_mode) {
      // use the theme color as the clear color
      ImVec4 bg_color = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
      Engine::Graphics::clear_screen({ bg_color.x * 0.6f, bg_color.y * 0.6f, bg_color.z * 0.6f, bg_color.w });
    }

    // When panel viewport mode changes, update virtual resolution for the new render target
    {
      static bool prev_panel_mode = Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().panel_viewport_mode;
      bool cur_panel_mode = Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().panel_viewport_mode;
      if (cur_panel_mode != prev_panel_mode) {
        if (!cur_panel_mode) {
          // Switched to direct window rendering — adapt virtual resolution to window aspect ratio
          int w = Engine::WindowManager::get_width();
          int h = Engine::WindowManager::get_height();
          float aspect = static_cast<float>(w) / static_cast<float>(h > 0 ? h : 1);
          Engine::Graphics::set_virtual_resolution(900.0f * aspect, 900.0f);
          Engine::Graphics::resize_post_processing(w, h);
        }
        prev_panel_mode = cur_panel_mode;
      }
    }

    // Toggle debug layer with F3 key
    if (Engine::Input::is_pressed_raw(Engine::KeyCode::F3))
    {
      Engine::ECS::is_layer_active("debug") ? Engine::ECS::disable_layer("debug") : Engine::ECS::enable_layer("debug");
    }

    // Toggle Fullscreen with F11 key
    // if (Engine::Input::is_pressed(Engine::KeyCode::F11))
    // {
    //   auto& settings = Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get_mutable();
    //   settings.fullscreen = !settings.fullscreen;
    //   Engine::WindowManager::set_fullscreen(settings.fullscreen);
    //   Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::save();
    // }
#endif
  }

  void register_panel([[maybe_unused]] const std::string& panel_name, [[maybe_unused]] const std::function<void()>& panel_func)
  {
#ifndef GAME_BUILD
    PanelInfo info;
    info.name = panel_name;
    info.render_func = panel_func;
    info.visible = true;
    registered_panels.push_back(info);
#endif
  }

  void clear_panels()
  {
#ifndef GAME_BUILD
    registered_panels.clear();
#endif
  }

  void set_style([[maybe_unused]] const std::string& style_name)
  {
#ifndef GAME_BUILD
    auto it = styles_map.find(style_name);
    if (it != styles_map.end())
    {
      it->second(); // Call the style setup function
    }

    // Update user settings
    Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::get_mutable().imgui_theme = style_name;
    Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::save();
#endif
  }

  void show_toast([[maybe_unused]] const std::string& message, [[maybe_unused]] ToastType type, [[maybe_unused]] float duration)
  {
#ifndef GAME_BUILD
    Toast toast;
    toast.message = message;
    toast.type = type;
    toast.duration = duration;
    toast.elapsed = 0.0f;
    active_toasts.push_back(toast);
#endif
  }

  bool is_viewport_mode_enabled()
  {
#ifndef GAME_BUILD
    return Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().panel_viewport_mode;
#else
    return false;
#endif
  }

  Math::Vec2D get_viewport_area_min()
  {
#ifndef GAME_BUILD
    if (is_viewport_mode_enabled()) {
      auto bounds = ViewportPanel::get_viewport_bounds();
      return { bounds.x, bounds.y };
    }
    else {
      ImGuiViewport* viewport = ImGui::GetMainViewport();
      return { viewport->WorkPos.x, viewport->WorkPos.y };
    }
#else
    return { 0.0f, 0.0f };
#endif
  }

  Math::Vec2D get_game_render_area_min()
  {
#ifndef GAME_BUILD
    if (is_viewport_mode_enabled()) {
      auto bounds = ViewportPanel::get_viewport_bounds();
      return { bounds.x, bounds.y };
    }
    else {
      if (main_dockspace_id != 0) {
        ImGuiDockNode* node = ImGui::DockBuilderGetCentralNode(main_dockspace_id);
        if (node) {
          return { node->Pos.x, node->Pos.y };
        }
      }
      ImGuiViewport* viewport = ImGui::GetMainViewport();
      return { viewport->WorkPos.x, viewport->WorkPos.y };
    }
#else
    return { 0.0f, 0.0f };
#endif
  }

  bool is_game_input_blocked()
  {
#ifndef GAME_BUILD
    // If the GUI is not visible, input is never blocked
    if (!gui_enabled || !main_gui_visible)
      return false;

    // Check if any ImGui window has focus
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (!ctx || !ctx->NavWindow)
      return false;

    // If the viewport panel is focused or hovered, allow game input.
    // Hover check is needed because focus transfers happen during ImGui render,
    // which runs AFTER ECS systems — so on the frame the user clicks the viewport
    // from another panel, focused is still false but hovered is true.
    if (ViewportPanel::is_viewport_focused() || ViewportPanel::is_viewport_hovered())
      return false;

    // Any other ImGui window is focused — block game input
    return true;
#else
    return false;
#endif
  }

  void save_layout([[maybe_unused]] const std::string& name)
  {
#ifndef GAME_BUILD
    std::filesystem::create_directories(LAYOUTS_DIR);
    std::string path = std::string(LAYOUTS_DIR) + name + ".ini";
    ImGui::SaveIniSettingsToDisk(path.c_str());
    current_layout_name = name;
#endif
  }

  void load_layout([[maybe_unused]] const std::string& name)
  {
#ifndef GAME_BUILD
    std::string path = std::string(LAYOUTS_DIR) + name + ".ini";
    if (!std::filesystem::exists(path))
    {
      show_toast("Layout '" + name + "' not found", ImGuiManager::ToastType::Warning, 3.0f);
      return;
    }
    ImGui::LoadIniSettingsFromDisk(path.c_str());
    current_layout_name = name;
#endif
  }

  std::vector<std::string> get_available_layouts()
  {
#ifndef GAME_BUILD
    std::vector<std::string> result;
    if (!std::filesystem::exists(LAYOUTS_DIR))
      return result;
    for (const auto& entry : std::filesystem::directory_iterator(LAYOUTS_DIR))
    {
      if (entry.is_regular_file() && entry.path().extension() == ".ini")
      {
        result.push_back(entry.path().stem().string());
      }
    }
    std::sort(result.begin(), result.end());
    return result;
#else
    return {};
#endif
  }

}
