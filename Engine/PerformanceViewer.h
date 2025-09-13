#pragma once
#include <imgui.h>
#include "Core.h"

inline void PerformanceViewerPanel()
{
  ImGui::Begin("Performance Viewer");

  if (Engine::CORE && Engine::CORE->getDt() > 0.0f)
  {
    ImGui::Text("FPS: %.1f", 1.0f / Engine::CORE->getDt());
  }

  ImGui::End();
}