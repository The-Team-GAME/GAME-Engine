#pragma once
#include <imgui.h>
#include "Core.h"
#include "Time.h"

inline void PerformanceViewerPanel()
{
  ImGui::Begin("Performance Viewer");

  static float frameTimeAccumulator = 0.0f;
  static int frameCount = 0;
  static float lastFPS = 0.0f;
  const float updateInterval = 0.1f; // Update FPS display every 0.5 seconds

  float dt = Engine::Time::get_real_dt();
  if (dt > 0.0f)
  {
    frameTimeAccumulator += dt;
    frameCount++;

    if (frameTimeAccumulator >= updateInterval)
    {
      lastFPS = Engine::Time::calc_fps(Engine::Time::get_real_dt());
      frameTimeAccumulator = 0.0f;
      frameCount = 0;
    }
  }

  ImGui::Text("FPS: %.1f", lastFPS);

  float tick_rate_input = Engine::Time::get_tick_rate();

  ImGui::SliderFloat("Tick Rate", &tick_rate_input, 10.0f, 144.0f);

  Engine::Time::set_tick_rate(tick_rate_input);

  ImGui::End();
}