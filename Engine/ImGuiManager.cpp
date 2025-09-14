#include "ImGuiManager.h"
#include "Core.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

void Engine::ImGuiManager::init()
{
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(Engine::CORE->getWindow(), true);
  ImGui_ImplOpenGL3_Init();
}

void Engine::ImGuiManager::begin()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void Engine::ImGuiManager::end()
{
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Engine::ImGuiManager::exit()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}
