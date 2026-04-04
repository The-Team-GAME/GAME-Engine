//-----------------------------------------------------------------------------------
// file:    FirebasePanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ImGui editor panel for managing the Firebase Cloud Firestore database.
//          Provides collection browsing, document viewing/editing, and an activity
//          log. All operations are async — the panel uses Engine::Firebase callbacks
//          to populate state that is rendered on the next frame.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

namespace FirebasePanel
{
    void draw_panel();
}


