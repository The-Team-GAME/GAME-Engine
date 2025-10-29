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
#include "Perf.h"
#include <algorithm>

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
  static s32 frame_count = 0;
  static const f32 update_interval = 0.25f;
  static std::unordered_map<std::string, double> cached_percentages;

  update_timer += Engine::Time::get_real_dt();
  frame_count++;

  if (update_timer >= update_interval) {
    // Calculate average FPS over the interval
    display_fps = frame_count / update_timer;

    // Update cached performance percentages
    auto total_time = Engine::Perf::get_total_time();
    cached_percentages.clear();
    for (const auto& [label, time] : Engine::Perf::get_elapsed_time_map()) {
      cached_percentages[label] = (time / total_time) * 100.0;
    }

    update_timer = 0.0f;
    frame_count = 0;
  }

  // FPS Display Section
  ImGui::SeparatorText("Frame Rate");
  ImGui::Text("FPS: %.1f", display_fps);

  // Color-code FPS based on performance
  if (display_fps >= 60.0f) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[GOOD]");
  }
  else if (display_fps >= 30.0f) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[OK]");
  }
  else {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[LOW]");
  }

  ImGui::Spacing();

  // Tick Rate Control Section
  ImGui::SeparatorText("Tick Rate Control");
  f32 tick_rate_input = Engine::Time::get_tick_rate();

  if (ImGui::SliderFloat("Tick Rate", &tick_rate_input, 45.0f, 144.0f, "%.1f Hz")) {
    Engine::Time::set_tick_rate(tick_rate_input);
  }

  ImGui::Text("Frame Time: %.2f ms", 1000.0f / tick_rate_input);

  ImGui::Spacing();

  // Performance Profiling Section
  if (!cached_percentages.empty()) {
    ImGui::SeparatorText("Performance Profiling");

    // Sort by percentage (descending)
    std::vector<std::pair<std::string, double>> sorted_percentages(
      cached_percentages.begin(), cached_percentages.end()
    );
    std::sort(sorted_percentages.begin(), sorted_percentages.end(),
      [](const auto& a, const auto& b) { return a.second > b.second; });

    for (const auto& [label, percentage] : sorted_percentages) {
      ImGui::Text("%s:", label.c_str());
      ImGui::SameLine(200.0f);
      ImGui::Text("%.2f%%", percentage);

      // Progress bar for visual representation
      ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
      ImGui::ProgressBar(static_cast<float>(percentage / 100.0), ImVec2(-1.0f, 0.0f), "");
      ImGui::PopStyleColor();
    }
  }

  ImGui::End();
}