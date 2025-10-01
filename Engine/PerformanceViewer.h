#pragma once
#include <imgui.h>
#include "Core.h"
#include "Time.h"

inline void PerformanceViewerPanel()
{
  ImGui::Begin("Performance Viewer");

  // Static variables to smooth the display updates
  static float display_fps = 0.0f;
  static float update_timer = 0.0f;
  static const float update_interval = 0.25f; // Update display every 0.25 seconds
  static std::unordered_map<std::string, double> cached_percentages;

  update_timer += Engine::Time::get_real_dt();

  if (update_timer >= update_interval) {
    display_fps = Engine::Time::calc_fps(Engine::Time::get_real_dt());

    // Update cached performance percentages
    auto total_time = Engine::PERFORMANCE.get_total_time();
    cached_percentages.clear();
    for (const auto& [label, time] : Engine::PERFORMANCE.get_elapsed_time_map()) {
      cached_percentages[label] = (time / total_time) * 100.0;
    }

    update_timer = 0.0f;
  }

  ImGui::Text("FPS: %.1f", display_fps);

  float tick_rate_input = Engine::Time::get_tick_rate();

  ImGui::SliderFloat("Tick Rate", &tick_rate_input, 10.0f, 144.0f);

  Engine::Time::set_tick_rate(tick_rate_input);


  for (const auto& [label, percentage] : cached_percentages) {
    ImGui::Text((label + ": " + std::to_string(percentage)).c_str());
  }

  ImGui::End();
}