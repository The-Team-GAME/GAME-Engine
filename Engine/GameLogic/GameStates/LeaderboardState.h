//-----------------------------------------------------------------------------------
// file:    LeaderboardState.h
// project: Monkey Stole My GPA
//
// brief:   A demonstration scene that fetches the "leaderboard" collection from
//          Firebase Firestore and renders each entry as on-screen text.
//          The fetch is triggered once on init and re-triggered whenever the
//          player presses R (Refresh).
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"
#include <string>
#include <vector>

class LeaderboardState : public Engine::Scene
{
public:
  void load()   override;
  void init()   override;
  void update() override;
  void draw()   override;
  void free()   override;
  void unload() override;

private:
  // ---- Rendering resources ----
  Engine::Graphics::Font font_title;
  Engine::Graphics::Font font_entry;

  // ---- Leaderboard data (populated from Firebase callback) ----
  struct Entry
  {
    std::string id;     // document ID
    std::string name;   // "name" field  (falls back to id if absent)
    int         score = 0;
  };
  std::vector<Entry> entries;

  // ---- State flags ----
  bool is_loading = false;    // true while async request is in flight
  bool has_loaded = false;    // true once first fetch completes
  std::string error_msg;      // non-empty on failure

  // ---- Internal helper ----
  void fetch_leaderboard();
};
