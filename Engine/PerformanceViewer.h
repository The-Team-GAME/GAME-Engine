#pragma once
#include <imgui.h>
#include "Core.h"

inline void PerformanceViewerPanel()
{
  ImGui::Begin("Performance Viewer");

  if (Engine::CORE)
  {
    static float frameTimeAccumulator = 0.0f;
    static int frameCount = 0;
    static float lastFPS = 0.0f;
    const float updateInterval = 0.5f; // Update FPS display every 0.5 seconds

    float dt = Engine::CORE->getDt();
    if (dt > 0.0f)
    {
      frameTimeAccumulator += dt;
      frameCount++;

      if (frameTimeAccumulator >= updateInterval)
      {
        lastFPS = static_cast<float>(frameCount) / frameTimeAccumulator;
        frameTimeAccumulator = 0.0f;
        frameCount = 0;
      }
    }

    ImGui::Text("FPS: %.1f", lastFPS);
  }

  ImGui::End();
}