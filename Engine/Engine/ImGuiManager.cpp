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

namespace {
  bool gui_enabled = true;
}

namespace ImguiMgr {

  void imgui_init()
  {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(Engine::WindowManager::get_window(), true); // TODO: pass window as parameter
    ImGui_ImplOpenGL3_Init();
  }

  void imgui_exit()
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  void begin()
  {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
  }

  void end()
  {
    ImGui::Render();
    if (gui_enabled)
    {
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
  }

  void update()
  {
    // Toggle ImGui with F1 key
    if (Engine::Input::is_key_pressed(GLFW_KEY_F1))
    {
      gui_enabled = !gui_enabled;
    }
  }
}
