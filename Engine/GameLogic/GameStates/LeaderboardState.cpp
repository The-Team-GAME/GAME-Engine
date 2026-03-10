//-----------------------------------------------------------------------------------
// file:    LeaderboardState.cpp
// project: Monkey Stole My GPA
//
// brief:   Fetches the "leaderboard" Firestore collection and draws each entry
//          sorted by score (descending) in the centre of the screen.
//          Press R to refresh, ESC to return to the main menu.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "LeaderboardState.h"
#include "../Engine/Firebase/FirebaseManager.h"

#include <rapidjson/document.h>
#include <algorithm>

// ============================================================================
// Lifecycle
// ============================================================================
void LeaderboardState::load()
{
  font_title = Engine::Resources::load_font("fonts/PlayfairDisplay-VariableFont_wght.ttf", 64);
  font_entry = Engine::Resources::load_font("fonts/Roboto-Regular.ttf", 36);
}

void LeaderboardState::init()
{
  entries.clear();
  error_msg.clear();
  has_loaded  = false;
  is_loading  = false;
  fetch_leaderboard();
}

void LeaderboardState::update()
{
  // R — refresh
  if (Engine::Input::is_pressed(Engine::KeyCode::R))
    fetch_leaderboard();

  // ESC — return to main menu
  if (Engine::Input::is_pressed(Engine::KeyCode::Escape))
    Engine::SceneManager::load_scene("MainMenuState");
}

void LeaderboardState::draw()
{
  Engine::Graphics::clear_screen({ 0.05f, 0.05f, 0.10f, 1.0f });

  const float screen_w = 1600.f;
  const float centre_x = screen_w * 0.5f;

  // ---- Title ----
  Engine::Graphics::draw_text(
    font_title,
    "Leaderboard",
    { centre_x - 200.f, 80.f },
    1.0f,
    { 1.f, 0.85f, 0.f, 1.f }   // gold
  );

  // ---- Sub-hint ----
  Engine::Graphics::draw_text(
    font_entry,
    "Press R to refresh   |   ESC to go back",
    { centre_x - 260.f, 155.f },
    0.5f,
    { 0.55f, 0.55f, 0.55f, 1.f }
  );

  // ---- Divider line ----
  Engine::Graphics::draw_line(
    { centre_x - 400.f, 180.f },
    { centre_x + 400.f, 180.f },
    2.f,
    { 0.3f, 0.3f, 0.3f, 1.f }
  );

  // ---- Body ----
  if (is_loading)
  {
    Engine::Graphics::draw_text(
      font_entry,
      "Loading...",
      { centre_x - 80.f, 280.f },
      1.0f,
      { 0.6f, 0.6f, 0.6f, 1.f }
    );
    return;
  }

  if (!error_msg.empty())
  {
    Engine::Graphics::draw_text(
      font_entry,
      "Error: " + error_msg,
      { centre_x - 300.f, 280.f },
      0.8f,
      { 1.f, 0.3f, 0.2f, 1.f }   // red
    );
    return;
  }

  if (entries.empty())
  {
    Engine::Graphics::draw_text(
      font_entry,
      has_loaded ? "No entries found." : "",
      { centre_x - 110.f, 280.f },
      1.0f,
      { 0.6f, 0.6f, 0.6f, 1.f }
    );
    return;
  }

  // ---- Column header ----
  const float col_rank  = centre_x - 380.f;
  const float col_name  = centre_x - 280.f;
  const float col_score = centre_x + 260.f;
  const float row_start = 210.f;
  const float row_step  = 50.f;

  Engine::Graphics::draw_text(font_entry, "#",     { col_rank,  row_start }, 0.7f, { 0.7f, 0.7f, 0.7f, 1.f });
  Engine::Graphics::draw_text(font_entry, "Name",  { col_name,  row_start }, 0.7f, { 0.7f, 0.7f, 0.7f, 1.f });
  Engine::Graphics::draw_text(font_entry, "Score", { col_score, row_start }, 0.7f, { 0.7f, 0.7f, 0.7f, 1.f });

  // ---- Entries ----
  const int max_visible = 12;
  for (int i = 0; i < static_cast<int>(entries.size()) && i < max_visible; ++i)
  {
    const auto& e   = entries[i];
    float y         = row_start + row_step + i * row_step;

    // Medal colours for top 3
    Math::Vec4D colour = { 1.f, 1.f, 1.f, 1.f };
    if (i == 0) colour = { 1.00f, 0.84f, 0.00f, 1.f };   // gold
    if (i == 1) colour = { 0.75f, 0.75f, 0.75f, 1.f };   // silver
    if (i == 2) colour = { 0.80f, 0.50f, 0.20f, 1.f };   // bronze

    Engine::Graphics::draw_text(font_entry, std::to_string(i + 1),       { col_rank,  y }, 0.8f, colour);
    Engine::Graphics::draw_text(font_entry, e.name,                       { col_name,  y }, 0.8f, colour);
    Engine::Graphics::draw_text(font_entry, std::to_string(e.score),      { col_score, y }, 0.8f, colour);
  }
}

void LeaderboardState::free()
{
  entries.clear();
}

void LeaderboardState::unload()
{
  font_title = {};
  font_entry = {};
}

// ============================================================================
// Firebase fetch
// ============================================================================
void LeaderboardState::fetch_leaderboard()
{
  if (is_loading) return;   // don't stack requests
  is_loading = true;
  error_msg.clear();

  Engine::Firebase::query_collection("leaderboard",
    [this](bool success, const std::string& data)
    {
      is_loading = false;
      has_loaded = true;

      if (!success)
      {
        error_msg = data;
        Log::error("[LeaderboardState] Fetch failed: {}", data);
        return;
      }

      // Parse JSON array: [{"_id":"...", "name":"...", "score":42}, ...]
      entries.clear();

      rapidjson::Document doc;
      doc.Parse(data.c_str());
      if (doc.HasParseError() || !doc.IsArray())
      {
        error_msg = "Invalid data received.";
        Log::error("[LeaderboardState] JSON parse error.");
        return;
      }

      for (rapidjson::SizeType i = 0; i < doc.Size(); ++i)
      {
        const auto& obj = doc[i];
        if (!obj.IsObject()) continue;

        Entry e;

        if (obj.HasMember("_id") && obj["_id"].IsString())
          e.id = obj["_id"].GetString();

        // "name" field (optional — fall back to document ID)
        if (obj.HasMember("name") && obj["name"].IsString())
          e.name = obj["name"].GetString();
        else
          e.name = e.id;

        // "score" field (int or double)
        if (obj.HasMember("score"))
        {
          const auto& sv = obj["score"];
          if (sv.IsInt())    e.score = sv.GetInt();
          else if (sv.IsInt64())  e.score = static_cast<int>(sv.GetInt64());
          else if (sv.IsDouble()) e.score = static_cast<int>(sv.GetDouble());
        }

        entries.push_back(e);
      }

      // Sort by score descending
      std::sort(entries.begin(), entries.end(),
        [](const Entry& a, const Entry& b) { return a.score > b.score; });

      Log::info("[LeaderboardState] Loaded {} entries.", entries.size());
    });
}
