//-----------------------------------------------------------------------------------
// file:    DiceMiniGamePanel.cpp
// project: Monkey Stole My GPA
//
// brief:   Dice Mini Game ImGui panel. Players enter a name and roll dice;
//          each roll is written to Firebase "leaderboard" collection.
//          An SSE watch keeps the displayed leaderboard in sync across all
//          running engine instances in real-time.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "../Precompiled.h"
#include "DiceMiniGamePanel.h"

#include "../Firebase/FirebaseManager.h"
#include "../Firebase/FirebaseAuth.h"
#include "../Time.h"

#include <imgui.h>
#include "LucideIcons.h"
#include "ViewportToolbar.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <ctime>
#include <cmath>

// ============================================================================
// Panel-local state
// ============================================================================
namespace
{
  // --- Player ---
  static char        player_name_buf[64] = "";
  static std::string active_player;      // Committed name

  // --- Dice ---
  static int   last_roll = 0;
  static int   my_score = 0;
  static bool  is_submitting = false;
  static float roll_anim_timer = 0.f;   // Fades dice face after a roll
  static constexpr float ROLL_ANIM_DURATION = 3.f;

  // --- Status bar ---
  static std::string status_msg;
  static ImVec4      status_color = { 1.f, 1.f, 1.f, 1.f };
  static float       status_timer = 0.f;

  // --- Leaderboard (updated via SSE) ---
  struct LeaderboardEntry
  {
    std::string player_name;
    int         score = 0;
    std::string timestamp;
  };
  static std::vector<LeaderboardEntry> leaderboard;
  static int  watch_id = -1;
  static bool is_fetching = false;

  // --- Live update flash ---
  static float live_flash_timer = 0.f;  // Briefly highlights the leaderboard header on each SSE event
  static int   live_update_count = 0;    // Cumulative count of SSE events received

  // --- Auth tracking ---
  static std::string last_auth_name;    // Display name last auto-set from Google auth

  // ================================================================
  // Helpers
  // ================================================================

  static std::string now_timestamp()
  {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    struct tm buf {};
    localtime_s(&buf, &t);
    char out[20];
    std::snprintf(out, sizeof(out), "%04d-%02d-%02d %02d:%02d:%02d",
      buf.tm_year + 1900, buf.tm_mon + 1, buf.tm_mday,
      buf.tm_hour, buf.tm_min, buf.tm_sec);
    return out;
  }

  static void set_status(const std::string& msg, bool success)
  {
    status_msg = msg;
    status_color = success
      ? ImVec4(0.2f, 1.f, 0.3f, 1.f)
      : ImVec4(1.f, 0.3f, 0.2f, 1.f);
    status_timer = 4.f;
  }

  static int roll_dice()
  {
    static std::mt19937 rng{ std::random_device{}() };
    static std::uniform_int_distribution<int> dist(1, 6);
    return dist(rng);
  }

  // Parse a collection response object {"playerA":{...},"playerB":{...}}
  static void parse_leaderboard(const std::string& json)
  {
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError() || !doc.IsObject()) return;

    leaderboard.clear();
    for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
    {
      if (!it->value.IsObject()) continue;

      LeaderboardEntry e;
      e.player_name = it->name.GetString();

      if (it->value.HasMember("score") && it->value["score"].IsNumber())
        e.score = static_cast<int>(it->value["score"].GetDouble());
      if (it->value.HasMember("timestamp") && it->value["timestamp"].IsString())
        e.timestamp = it->value["timestamp"].GetString();

      leaderboard.push_back(std::move(e));
    }

    // Sort highest score first
    std::sort(leaderboard.begin(), leaderboard.end(),
      [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
        return a.score > b.score;
      });
  }

  static void fetch_leaderboard()
  {
    if (is_fetching) return;
    is_fetching = true;
    Engine::Firebase::query_collection("leaderboard",
      [](bool ok, const std::string& data) {
        is_fetching = false;
        if (ok) parse_leaderboard(data);
      });
  }

  static void start_leaderboard_watch()
  {
    if (watch_id >= 0) return;
    watch_id = Engine::Firebase::watch_collection("leaderboard",
      [](bool /*ok*/, const std::string& /*event_json*/) {
        // Any change → flash the indicator and re-fetch the full leaderboard
        live_flash_timer = 2.5f;
        ++live_update_count;
        fetch_leaderboard();
      });
  }

  static void submit_score()
  {
    if (active_player.empty() || is_submitting) return;

    is_submitting = true;

    rapidjson::StringBuffer sbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sbuf);
    writer.StartObject();
    writer.Key("player_name");  writer.String(active_player.c_str());
    writer.Key("score");        writer.Int(my_score);
    writer.Key("timestamp");    writer.String(now_timestamp().c_str());
    writer.EndObject();

    std::string json = sbuf.GetString();
    std::string name = active_player;
    int         score = my_score;

    Engine::Firebase::set_document("leaderboard", name, json,
      [score](bool ok, const std::string& err) {
        is_submitting = false;
        if (ok)
          set_status("Score " + std::to_string(score) + " submitted!", true);
        else
          set_status("Submit failed: " + err, false);
      });
  }

  // Lucide dice icons (Dice1–Dice6)
  static const char* dice_icon(int n)
  {
    switch (n)
    {
    case 1: return LucideIcons::Dice1;
    case 2: return LucideIcons::Dice2;
    case 3: return LucideIcons::Dice3;
    case 4: return LucideIcons::Dice4;
    case 5: return LucideIcons::Dice5;
    case 6: return LucideIcons::Dice6;
    default: return LucideIcons::Dices;
    }
  }

} // anonymous namespace

// ============================================================================
// draw_panel
// ============================================================================
void DiceMiniGamePanel::draw_panel()
{
  ImGui::Begin("Dice Mini Game");

  const float dt = Engine::Time::get_real_dt();

  // Tick timers
  if (status_timer > 0.f) status_timer -= dt;
  if (roll_anim_timer > 0.f) roll_anim_timer -= dt;
  if (live_flash_timer > 0.f) live_flash_timer -= dt;

  // Auto-start the leaderboard watch on first configured frame
  if (watch_id < 0 && Engine::Firebase::is_configured())
    start_leaderboard_watch();

  // Auto-set player name from Google sign-in when authenticated
  {
    if (Engine::Firebase::Auth::is_authenticated())
    {
      std::string disp = Engine::Firebase::Auth::get_display_name();
      if (disp.empty()) disp = Engine::Firebase::Auth::get_email();
      if (!disp.empty() && disp != last_auth_name)
      {
        last_auth_name = disp;
        active_player = disp;
        my_score = 0;
        last_roll = 0;
        fetch_leaderboard();
      }
    }
    else if (!last_auth_name.empty())
    {
      // Signed out — allow manual input again
      last_auth_name.clear();
    }
  }

  // ======================================================================
  // Player Setup
  // ======================================================================
  if (ImGui::CollapsingHeader("Player Setup", ImGuiTreeNodeFlags_DefaultOpen))
  {
    if (Engine::Firebase::Auth::is_authenticated())
    {
      ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.f, 1.f), "[Google] Signed in — player name set automatically");
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.05f, 0.05f, 1.f));
      if (ImGui::SmallButton("Sign Out"))
      {
        Engine::Firebase::Auth::sign_out();
        active_player.clear();
        my_score = 0;
        last_roll = 0;
      }
      ImGui::PopStyleColor(3);
    }
    else
    {
      ImGui::SetNextItemWidth(200.f);
      ImGui::InputText("##name", player_name_buf, sizeof(player_name_buf));
      ImGui::SameLine();
      if (ImGui::Button("Set Name"))
      {
        // Trim surrounding spaces
        std::string trimmed = player_name_buf;
        while (!trimmed.empty() && trimmed.front() == ' ') trimmed.erase(trimmed.begin());
        while (!trimmed.empty() && trimmed.back() == ' ') trimmed.pop_back();

        if (!trimmed.empty())
        {
          active_player = trimmed;
          my_score = 0;
          last_roll = 0;
          set_status("Player set to: " + active_player, true);
          fetch_leaderboard();
        }
      }
      ImGui::SameLine();
      ImGui::TextDisabled("Player Name");

      ImGui::Spacing();
      ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.f, 1.f), "(i)");
      ImGui::SameLine();
      ImGui::TextWrapped("Sign in with Google (Firebase panel > Authentication) to have your name filled in automatically.");
    }

    if (!active_player.empty())
    {
      ImGui::Text("Active Player:"); ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.2f, 1.f, 0.3f, 1.f), "%s", active_player.c_str());

      ImGui::Text("Total Score:  "); ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f), "%d pts", my_score);
    }
    else
    {
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f), "Enter a name and press Set Name to begin.");
    }
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // ======================================================================
  // Dice Roll
  // ======================================================================
  if (ImGui::CollapsingHeader("Dice Roll", ImGuiTreeNodeFlags_DefaultOpen))
  {
    const bool disabled = active_player.empty() || is_submitting;
    if (disabled) ImGui::BeginDisabled();

    if (ImGui::Button("Roll Dice!", ImVec2(120.f, 40.f)))
    {
      last_roll = roll_dice();
      my_score += last_roll;
      roll_anim_timer = ROLL_ANIM_DURATION;
      submit_score();
    }

    if (disabled) ImGui::EndDisabled();

    if (active_player.empty())
      ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f), "Set a player name first.");

    if (is_submitting)
    {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f), "Submitting...");
    }

    // Dice result display
    if (last_roll > 0)
    {
      ImGui::Spacing();

      // Fade the highlight after the animation window
      const float bright = roll_anim_timer > 0.f ? 1.f : 0.55f;
      ImGui::TextColored(ImVec4(1.f, 0.85f * bright, 0.f, 1.f), "You rolled:");
      ImGui::SameLine();
      // Draw the Lucide dice icon in the icon font
      if (auto* font = static_cast<ImFont*>(ViewportToolbar::get_icon_font()))
      {
        ImGui::PushFont(font);
        ImGui::TextColored(ImVec4(1.f, 0.5f * bright, 0.1f, 1.f), "%s", dice_icon(last_roll));
        ImGui::PopFont();
      }
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.f, 0.5f * bright, 0.1f, 1.f), " %d", last_roll);
    }
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // ======================================================================
  // Live Leaderboard
  // ======================================================================
  if (ImGui::CollapsingHeader("Live Leaderboard", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // Connection indicator
    ImGui::Text("Live Update:");
    ImGui::SameLine();
    if (watch_id >= 0)
    {
      if (live_flash_timer > 0.f)
      {
        // Pulse between green and cyan each time an SSE push fires
        float pulse = static_cast<float>(std::sin(ImGui::GetTime() * 12.0) * 0.5 + 0.5);
        ImGui::TextColored(ImVec4(0.1f, 1.f, 0.5f + 0.5f * pulse, 1.f),
          "** LIVE UPDATE #%d received! **", live_update_count);
      }
      else
        ImGui::TextColored(ImVec4(0.2f, 1.f, 0.3f, 1.f), "connected");
    }
    else
      ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f), "connecting...");

    ImGui::SameLine();
    if (is_fetching)
      ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f), " (refreshing...)");

    ImGui::SameLine();
    if (ImGui::SmallButton("Refresh"))
      fetch_leaderboard();

    ImGui::Spacing();

    if (leaderboard.empty())
    {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f),
        "No entries yet. Roll the dice to add yours!");
    }
    else
    {
      constexpr ImGuiTableFlags table_flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;

      if (ImGui::BeginTable("leaderboard_tbl", 4, table_flags, ImVec2(0.f, 200.f)))
      {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 40.f);
        ImGui::TableSetupColumn("Player", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Score", ImGuiTableColumnFlags_WidthFixed, 60.f);
        ImGui::TableSetupColumn("Last Roll", ImGuiTableColumnFlags_WidthFixed, 160.f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < static_cast<int>(leaderboard.size()); ++i)
        {
          const auto& e = leaderboard[i];
          const bool  is_me = (e.player_name == active_player);

          ImGui::TableNextRow();

          // Rank column (gold for #1)
          ImGui::TableSetColumnIndex(0);
          if (i == 0)
            ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f), "#%d", i + 1);
          else
            ImGui::Text("#%d", i + 1);

          // Player name (green if it's the local player)
          ImGui::TableSetColumnIndex(1);
          if (is_me)
            ImGui::TextColored(ImVec4(0.2f, 1.f, 0.3f, 1.f), "%s", e.player_name.c_str());
          else
            ImGui::TextUnformatted(e.player_name.c_str());

          // Score
          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%d", e.score);

          // Timestamp
          ImGui::TableSetColumnIndex(3);
          ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f), "%s", e.timestamp.c_str());
        }

        ImGui::EndTable();
      }
    }
  }

  ImGui::Spacing();

  // Status bar
  if (status_timer > 0.f && !status_msg.empty())
    ImGui::TextColored(status_color, "%s", status_msg.c_str());

  ImGui::End();
}
