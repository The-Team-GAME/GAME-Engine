//-----------------------------------------------------------------------------------
// file:    PerformanceViewer.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines an ImGui panel for visualizing performance metrics
//          including FPS, tick rate control, and performance profiling data as
//          percentages of total execution time.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include <imgui.h>
#include "Core.h"
#include "Time.h"

/*=================================================================================
Function Brief:
Creates and renders an ImGui panel that displays performance metrics including
current FPS, adjustable tick rate, and performance profiling data showing the
percentage of time spent in each labeled code section.

Where to call it:
Call this function once per frame during the ImGui rendering phase, typically
in the Draw() or Update() function of a game state.
==================================================================================*/
inline void PerformanceViewerPanel()
{
  ImGui::Begin("Performance Viewer");

  // Static variables to smooth the display updates
  static f32 display_fps = 0.0f;
  static f32 update_timer = 0.0f;
  static const f32 update_interval = 0.25f; // Update display every 0.25 seconds
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

  f32 tick_rate_input = Engine::Time::get_tick_rate();

  ImGui::SliderFloat("Tick Rate", &tick_rate_input, 45.0f, 144.0f);

  Engine::Time::set_tick_rate(tick_rate_input);


  for (const auto& [label, percentage] : cached_percentages) {
    ImGui::Text((label + ": " + std::to_string(percentage)).c_str());
  }

  ImGui::End();
}