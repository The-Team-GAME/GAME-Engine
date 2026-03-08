//-----------------------------------------------------------------------------------
// file:    UpgradePickerUI.h
// project: Monkey Stole My GPA
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

  auto get_upgrade_sprite_index = [](const UpgradeDefinition& upg) -> int {
    if (upg.stat_name == "base_attack_damage" || upg.stat_name == "damage") {
      return 0;
    }
    if (upg.stat_name == "uppercut_damage" || upg.stat_name == "uppercut damage") {
      return 1;
    }
    if (upg.stat_name == "smash_damage" || upg.stat_name == "smash damage") {
      return 2;
    }
    if (upg.stat_name == "uppercut_cooldown") {
      return 3;
    }
    if (upg.stat_name == "smash_cooldown") {
      return 4;
    }
    if (upg.stat_name == "dash_cooldown") {
      return 5;
    }
    return 0;
  };

  auto build_effect_text = [](const UpgradeDefinition& upg) -> std::string {
    if (!upg.effect_text.empty()) {
      return upg.effect_text;
    }

    char buffer[128]{};
    if (upg.type == UpgradeType::Flat) {
      snprintf(buffer, sizeof(buffer), "+%.1f  %s", upg.value, upg.stat_name.c_str());
    }
    else {
      snprintf(buffer, sizeof(buffer), "x%.2f  %s", upg.value, upg.stat_name.c_str());
    }
    return buffer;
  };

  auto draw_card_sprite_button = [](Engine::Graphics::Texture texture, int sprite_index, const char* id, float max_width, float max_height, float& hover_lerp) -> bool {
    if (!texture || texture->id == 0 || texture->width <= 0 || texture->height <= 0) {
      return false;
    }

    constexpr int sprite_columns = 6;
    const int clamped_index = (sprite_index < 0) ? 0 : (sprite_index >= sprite_columns ? sprite_columns - 1 : sprite_index);

    const float sprite_width = static_cast<float>(texture->width) / static_cast<float>(sprite_columns);
    const float sprite_height = static_cast<float>(texture->height);

    const float uv_left = (sprite_width * static_cast<float>(clamped_index)) / static_cast<float>(texture->width);
    const float uv_right = (sprite_width * static_cast<float>(clamped_index + 1)) / static_cast<float>(texture->width);

    const float available_width = ImGui::GetContentRegionAvail().x;
    const float target_width = (available_width > 0.0f) ? std::min(max_width, available_width) : max_width;

    float base_width = target_width;
    float base_height = base_width * (sprite_height / sprite_width);

    if (base_height > max_height) {
      base_height = max_height;
      base_width = base_height * (sprite_width / sprite_height);
    }

    const float scale = 1.0f + 0.08f * hover_lerp;
    float draw_width = base_width * scale;
    float draw_height = base_height * scale;

    if (draw_width > available_width && draw_width > 0.0f) {
      const float clamp_scale = available_width / draw_width;
      draw_width *= clamp_scale;
      draw_height *= clamp_scale;
    }
    if (draw_height > max_height * 1.08f && draw_height > 0.0f) {
      const float clamp_scale = (max_height * 1.08f) / draw_height;
      draw_width *= clamp_scale;
      draw_height *= clamp_scale;
    }

    const float horizontal_padding = std::max(0.0f, (target_width - draw_width) * 0.5f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + horizontal_padding);

    ImGui::PushID(id);
    const bool pressed = ImGui::InvisibleButton("##card_image_button", ImVec2(draw_width, draw_height));
    const bool hovered = ImGui::IsItemHovered();
    hover_lerp += (hovered ? 1.0f : -1.0f) * ImGui::GetIO().DeltaTime * 10.0f;
    if (hover_lerp < 0.0f) hover_lerp = 0.0f;
    if (hover_lerp > 1.0f) hover_lerp = 1.0f;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddImage(
      (ImTextureID)(intptr_t)texture->id,
      ImGui::GetItemRectMin(),
      ImGui::GetItemRectMax(),
      ImVec2(uv_left, 0.0f),
      ImVec2(uv_right, 1.0f),
      IM_COL32(255, 255, 255, 255)
    );
    ImGui::PopID();

    return pressed;
  };

  static Engine::Graphics::Texture s_card_spritesheet = Engine::Resources::load_texture("images/Cards_Spritesheet.png");

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

    const float card_w = 500.0f;
    const float card_h = 600.0f;
    int         pick_slot = -1;
    static float s_card_hover_lerp[2] = { 0.0f, 0.0f };

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

        // Card art from shared spritesheet
        std::string image_button_id = "card_image_" + std::to_string(c);
        if (draw_card_sprite_button(s_card_spritesheet, get_upgrade_sprite_index(upg), image_button_id.c_str(), card_w - 24.0f, 340.0f, s_card_hover_lerp[c])) {
          pick_slot = c;
        }
        if (s_card_spritesheet && s_card_spritesheet->id != 0) {
          ImGui::Spacing();
        }
        else {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.9f, 1.0f));
          ImGui::TextWrapped("%s", upg.display_name.c_str());
          ImGui::PopStyleColor();
        }

        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.9f, 1.0f));
        ImGui::TextWrapped("%s", upg.display_name.c_str());
        ImGui::PopStyleColor();

        const std::string effect_text = build_effect_text(upg);
        ImGui::TextWrapped("%s", effect_text.c_str());

        if (!upg.description.empty()) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78f, 0.78f, 0.78f, 1.0f));
          ImGui::TextWrapped("%s", upg.description.c_str());
          ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        // Level
        ImGui::TextDisabled("Level: %d / %d", cur_lvl, upg.max_level);

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
