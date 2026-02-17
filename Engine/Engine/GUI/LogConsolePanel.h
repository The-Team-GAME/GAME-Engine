//-----------------------------------------------------------------------------------
// file:    LogConsolePanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   In-engine log console panel for viewing Log::info/warning/error
//          output directly inside the editor with filtering and color coding.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <imgui.h>
#include "../Logger.h"

inline void LogConsolePanel()
{
  ImGui::Begin("Log Console");

  // ── Toolbar row ──────────────────────────────────────────────
  static int filter = 0; // 0=All, 1=Info, 2=Warning, 3=Error
  ImGui::SetNextItemWidth(100.0f);
  ImGui::Combo("##Filter", &filter, "All\0Info\0Warning\0Error\0");
  ImGui::SameLine();

  static bool auto_scroll = true;
  ImGui::Checkbox("Auto-scroll", &auto_scroll);
  ImGui::SameLine();

  if (ImGui::Button("Clear"))
  {
    Log::clear_log();
  }

  ImGui::Separator();

  // ── Log entries ──────────────────────────────────────────────
  ImGui::BeginChild("##LogScrollRegion", ImVec2(0, 0), false,
    ImGuiWindowFlags_HorizontalScrollbar);

  auto entries = Log::get_log_entries();

  for (const auto& entry : entries)
  {
    // Apply filter
    if (filter == 1 && entry.level != Log::Level::Info) continue;
    if (filter == 2 && entry.level != Log::Level::Warning) continue;
    if (filter == 3 && entry.level != Log::Level::Error && entry.level != Log::Level::Crash) continue;

    // Pick color
    ImVec4 color;
    const char* prefix;
    switch (entry.level)
    {
    case Log::Level::Info:
      color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
      prefix = "[INFO]   ";
      break;
    case Log::Level::Warning:
      color = ImVec4(1.0f, 0.85f, 0.2f, 1.0f);
      prefix = "[WARN]   ";
      break;
    case Log::Level::Error:
      color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
      prefix = "[ERROR]  ";
      break;
    case Log::Level::Crash:
      color = ImVec4(1.0f, 0.2f, 0.8f, 1.0f);
      prefix = "[CRASH]  ";
      break;
    default:
      color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
      prefix = "         ";
      break;
    }

    // Timestamp + colored message
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[%7.1f]", entry.timestamp);
    ImGui::SameLine();
    ImGui::TextColored(color, "%s%s", prefix, entry.message.c_str());
  }

  if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f)
    ImGui::SetScrollHereY(1.0f);

  ImGui::EndChild();

  ImGui::End();
}
