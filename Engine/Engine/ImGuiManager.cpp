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

namespace {
  bool gui_enabled = true;
  std::vector<std::function<void()>> registered_panels;
}

namespace ImGuiManager {

  void init()
  {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable Multi-Viewport / Platform Windows

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

      // Create dockspace
      ImGuiViewport* viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowPos(viewport->WorkPos);
      ImGui::SetNextWindowSize(viewport->WorkSize);
      ImGui::SetNextWindowViewport(viewport->ID);

      ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
      window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
      window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
      window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
      window_flags |= ImGuiWindowFlags_NoBackground;

      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

      ImGui::Begin("DockSpace", nullptr, window_flags);
      ImGui::PopStyleVar(3);

      // DockSpace
      ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

      // Add a menu bar
      if (ImGui::BeginMenuBar())
      {
        if (ImGui::BeginMenu("Windows"))
        {
          ImGui::MenuItem("Toggle GUI (F1)", nullptr, &gui_enabled);
          ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
      }

      ImGui::End();

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
    if (Engine::Input::is_key_pressed(GLFW_KEY_F1))
    {
      gui_enabled = !gui_enabled;
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
}
