//-----------------------------------------------------------------------------------
// file:    UpgradePickerUI.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   In-game upgrade picker overlay. Every frame:
//            - If an upgrade is available, draws a 64x64 Upgrade.png button at
//              the bottom-right of the screen via ImGui (no ECS entity needed).
//            - When clicked, rolls cards (if not already rolled) and signals the
//              modal to open.
//            - The modal shows current upgrade levels on the left and the two
//              rolled cards on the right. Picking or skipping resumes time.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <imgui.h>
#include <string>
#include "../FileIO/Settings.h"
#include "../FileIO/SettingsManager.h"
#include "../UpgradeManager.h"
#include "../Resources.h"
#include "../Time.h"

inline void UpgradePickerUI()
{
  using namespace Engine::FileIO;

  auto& mgr = Engine::UpgradeManager::instance();
  auto& settings = SettingsManager<UpgradeSettings>::get_mutable();

  // ---- Rarity color helper ----
  auto rarity_color = [&](const std::string& rarity) -> ImVec4 {
    constexpr ImVec4 tier_colors[] = {
      {0.75f, 0.75f, 0.75f, 1.0f},  // 0 Common  — grey
      {0.30f, 0.55f, 1.00f, 1.0f},  // 1 Rare    — blue
      {0.70f, 0.30f, 1.00f, 1.0f},  // 2 Epic    — purple
      {1.00f, 0.75f, 0.00f, 1.0f},  // 3 Legendary — gold
      {1.00f, 0.30f, 0.30f, 1.0f},  // 4 Mythic  — red
      {0.00f, 1.00f, 0.80f, 1.0f},  // 5+ teal
    };
    for (int i = 0; i < static_cast<int>(settings.rarities.size()); ++i)
      if (settings.rarities[i].name == rarity)
        return tier_colors[i < 6 ? i : 5];
    return { 0.55f, 0.55f, 0.55f, 1.0f };
    };

  // =========================================================================
  // Open modal when signaled
  // =========================================================================
  if (mgr.consume_upgrade_picker_request())
    ImGui::OpenPopup("##UpgradePickerModal");

  // =========================================================================
  // Card-pick modal
  // =========================================================================
  static bool s_was_paused = false;

  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
    ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(800, 0), ImGuiCond_Appearing);
  ImGui::SetNextWindowBgAlpha(0.95f);

  if (ImGui::BeginPopupModal("##UpgradePickerModal", nullptr,
    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
  {
    // Slow time while choosing (can't be exactly 0 or input breaks)
    if (!s_was_paused)
    {
      Engine::Time::set_time_scale(0.0001f);
      s_was_paused = true;
    }

    // ---- Header ----
    ImGui::Spacing();
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("  UPGRADE AVAILABLE  ").x) * 0.5f);
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.25f, 1.0f), "  UPGRADE AVAILABLE  ");
    ImGui::SameLine();
    ImGui::TextDisabled("  Points: %d / %d", mgr.get_points(), mgr.get_next_threshold());
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // =========================================================================
    // LEFT column — current upgrade levels
    // =========================================================================
    ImGui::BeginChild("##upg_history", ImVec2(210, 360), true);
    ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "Your Upgrades");
    ImGui::Separator();
    ImGui::Spacing();

    bool has_any = false;
    for (auto& upg : settings.upgrades)
    {
      int lvl = mgr.get_upgrade_level(upg.id);
      if (lvl <= 0) continue;
      has_any = true;

      ImVec4 rc = rarity_color(upg.rarity);
      ImGui::PushStyleColor(ImGuiCol_Text, rc);
      ImGui::TextUnformatted(upg.display_name.c_str());
      ImGui::PopStyleColor();
      ImGui::SameLine();
      ImGui::TextDisabled(" Lv%d/%d", lvl, upg.max_level);
    }
    if (!has_any)
      ImGui::TextDisabled("None yet.");

    ImGui::EndChild();

    ImGui::SameLine(0.0f, 12.0f);

    // =========================================================================
    // RIGHT — rolled cards
    // =========================================================================
    ImGui::BeginGroup();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, 1.0f), "Choose a card:");
    ImGui::Spacing();

    const float card_w = 270.0f;
    const float card_h = 220.0f;
    int         pick_slot = -1;

    for (int c = 0; c < 2; ++c)
    {
      if (c == 1) ImGui::SameLine(0.0f, 10.0f);

      int idx = mgr.get_rolled_card(c);
      ImGui::BeginGroup();

      if (idx >= 0 && idx < static_cast<int>(settings.upgrades.size()))
      {
        auto& upg = settings.upgrades[idx];
        int   cur_lvl = mgr.get_upgrade_level(upg.id);
        ImVec4 rc = rarity_color(upg.rarity);

        // Card background tinted with rarity colour
        ImGui::PushStyleColor(ImGuiCol_ChildBg,
          ImVec4(rc.x * 0.12f, rc.y * 0.12f, rc.z * 0.12f, 1.0f));
        ImGui::BeginChild(("##card" + std::to_string(c)).c_str(),
          ImVec2(card_w, card_h), true);

        // Rarity badge
        ImGui::TextColored(rc, "[ %s ]", upg.rarity.c_str());
        ImGui::Spacing();

        // Name
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.9f, 1.0f));
        ImGui::TextWrapped("%s", upg.display_name.c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // Effect
        if (upg.type == UpgradeType::Flat)
          ImGui::Text("  +%.1f  %s", upg.value, upg.stat_name.c_str());
        else
          ImGui::Text("  x%.2f  %s", upg.value, upg.stat_name.c_str());

        // Level
        ImGui::TextDisabled("Level: %d / %d", cur_lvl, upg.max_level);

        // Pick button — tinted with rarity colour
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button,
          ImVec4(rc.x * 0.4f, rc.y * 0.4f, rc.z * 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
          ImVec4(rc.x * 0.65f, rc.y * 0.65f, rc.z * 0.65f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
          ImVec4(rc.x * 0.9f, rc.y * 0.9f, rc.z * 0.9f, 1.0f));
        std::string btn_label = "  Pick!  ##" + std::to_string(c);
        if (ImGui::Button(btn_label.c_str(), ImVec2(-1.0f, 0.0f)))
          pick_slot = c;
        ImGui::PopStyleColor(3);

        ImGui::EndChild();
        ImGui::PopStyleColor(); // ChildBg
      }
      else
      {
        ImGui::BeginChild(("##emptycard" + std::to_string(c)).c_str(),
          ImVec2(card_w, card_h), true);
        ImGui::TextDisabled("(no eligible card)");
        ImGui::EndChild();
      }

      ImGui::EndGroup();
    }

    ImGui::EndGroup();

    // ---- Apply pick / close ----
    if (pick_slot >= 0)
    {
      mgr.pick_card(pick_slot);
      Engine::Time::set_time_scale(1.0f);
      s_was_paused = false;
      ImGui::CloseCurrentPopup();
    }

    // ---- Skip footer ----
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button("Skip", ImVec2(90.0f, 0.0f)))
    {
      mgr.skip_cards();
      Engine::Time::set_time_scale(1.0f);
      s_was_paused = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Discard both cards and lose this upgrade opportunity.");
    ImGui::Spacing();

    ImGui::EndPopup();
  }
  else if (s_was_paused)
  {
    // Modal was dismissed without picking (e.g. clicked outside) — skip & resume
    mgr.skip_cards();
    Engine::Time::set_time_scale(1.0f);
    s_was_paused = false;
  }
}


