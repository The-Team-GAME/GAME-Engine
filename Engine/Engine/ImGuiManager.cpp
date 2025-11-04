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
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "InputManager.h"
#include "Perf.h"
#include "ImGuiThemes.h"
#include "FileIO/Settings.h"
#include "Core.h"
#include "SceneManager.h"

namespace {
  bool gui_enabled = true;

  struct PanelInfo {
    std::string name;
    std::function<void()> render_func;
    bool visible = true;
  };

  std::vector<PanelInfo> registered_panels;

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
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
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

    ImGui_ImplGlfw_InitForOpenGL(Engine::WindowManager::get_window(), true);
    ImGui_ImplOpenGL3_Init();
  }

  void exit()
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  void begin()
  {
    if (gui_enabled)
    {
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

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
          if (ImGui::BeginMenu("ImGui"))
          {
            ImGui::MenuItem("Toggle GUI", "F1", &gui_enabled);
            ImGui::MenuItem("ImGui Viewport Mode", "F2", &Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get_mutable().imgui_viewport_enabled);

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
        bool& is_paused = Engine::Core::is_paused();


        // Play/Pause toggle button
        if (is_paused) {
          if (ImGui::Button("Play", ImVec2(80, 0))) {
            is_paused = false;
          }
        }
        else {
          if (ImGui::Button("Pause", ImVec2(80, 0))) {
            is_paused = true;
          }
        }

        ImGui::SameLine();

        // Stop button 
        if (ImGui::Button("Stop", ImVec2(60, 0))) {
          std::string current = Engine::SceneManager::get_current_scene_name();
          Engine::SceneManager::unload_current_scene();
          Engine::SceneManager::load_scene(current);
          is_paused = true;
          // Force one update to process deferred entity destruction
          Engine::Core::request_forced_update();
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
  }

  void end()
  {
    Engine::Perf::start("ImGui Render");

    if (gui_enabled)
    {
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
    }

    Engine::Perf::stop("ImGui Render");
  }

  void update()
  {
    // Toggle ImGui with F1 key
    if (Engine::Input::is_pressed(Engine::KeyCode::F1))
    {
      gui_enabled = !gui_enabled;
    }

    // Toggle ImGui Viewport Mode with F2 key
    if (Engine::Input::is_pressed(Engine::KeyCode::F2))
    {
      Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get_mutable().imgui_viewport_enabled =
        !Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().imgui_viewport_enabled;
    }

    // Make sure that imgui_viewport_enabled is false when gui is disabled
    if (!gui_enabled) Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get_mutable().imgui_viewport_enabled = false;

    // Clear screen every frame when imgui_viewport_enabled is true
    if (Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().imgui_viewport_enabled) {
      // use the theme color as the clear color
      ImVec4 bg_color = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
      Engine::Graphics::clear_screen({ bg_color.x * 0.6f, bg_color.y * 0.6f, bg_color.z * 0.6f, bg_color.w });
    }

  }

  void register_panel(const std::string& panel_name, const std::function<void()>& panel_func)
  {
    PanelInfo info;
    info.name = panel_name;
    info.render_func = panel_func;
    info.visible = true;
    registered_panels.push_back(info);
  }

  void clear_panels()
  {
    registered_panels.clear();
  }

  void set_style(const std::string& style_name)
  {
    auto it = styles_map.find(style_name);
    if (it != styles_map.end())
    {
      it->second(); // Call the style setup function
    }

    // Update user settings
    Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::get_mutable().imgui_theme = style_name;
    Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::save();
  }

}
