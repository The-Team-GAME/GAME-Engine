#include "ImGuiManager.h"
#include "Core.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "InputManager.h"

namespace {
  bool guiEnabled = true;
}

namespace ImguiMgr {

  void imgui_init()
  {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(Engine::WINDOW.getWindow(), true); // TODO: pass window as parameter
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
    if (guiEnabled)
    {
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
  }

  void update()
  {
    // Toggle ImGui with F1 key
    if (Engine::Input::IsKeyPressed(GLFW_KEY_F1))
    {
      guiEnabled = !guiEnabled;
    }
  }
}
