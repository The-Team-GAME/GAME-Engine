#include "ImGuiManager.h"
#include "Core.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

std::vector<Engine::ImGuiManager::Panel> Engine::ImGuiManager::panels;

void Engine::ImGuiManager::init()
{
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(Engine::CORE->getWindow(), true);
  ImGui_ImplOpenGL3_Init();
}

void Engine::ImGuiManager::update()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  for (auto &panel : panels)
  {
    panel();
  }

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Engine::ImGuiManager::addPanel(Panel panel)
{
  panels.push_back(panel);
}