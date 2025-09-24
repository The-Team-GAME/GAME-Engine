#pragma once
#include <imgui.h>
#include "Core.h"

inline void PerformanceViewerPanel(f32& target_fps)
{
  ImGui::Begin("Performance Viewer");

  static float frameTimeAccumulator = 0.0f;
  static int frameCount = 0;
  static float lastFPS = 0.0f;
  const float updateInterval = 0.5f; // Update FPS display every 0.5 seconds

  float dt = Engine::Time::get_fixed_dt();
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

  ImGui::SliderFloat("Target FPS", &target_fps, 10.0f, 3000.0f);

  ImGui::End();
}