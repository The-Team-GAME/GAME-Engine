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
  std::vector<std::function<void()>> registered_panels;

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
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

        bool& is_paused = Engine::Core::is_paused();

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        // Play/Pause toggle button
        if (is_paused) {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 0.6f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.8f, 0.2f, 0.8f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.9f, 0.2f, 1.0f));
          if (ImGui::Button("Play", ImVec2(80, 0))) {
            is_paused = false;
          }
        }
        else {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.1f, 0.6f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.7f, 0.1f, 0.8f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.8f, 0.1f, 1.0f));
          if (ImGui::Button("Pause", ImVec2(80, 0))) {
            is_paused = true;
          }
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        // Stop button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));

        if (ImGui::Button("Stop", ImVec2(60, 0))) {
          std::string current = Engine::SceneManager::get_current_scene_name();
          Engine::SceneManager::unload_current_scene();
          Engine::SceneManager::load_scene(current);
          is_paused = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::PopStyleVar();

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
        panel();
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
    if (!gui_enabled)
    {
      Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get_mutable().imgui_viewport_enabled = false;
    }
  }

  void register_panel(const std::function<void()>& panel_func)
  {
    registered_panels.push_back(panel_func);
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
