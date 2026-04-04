//-----------------------------------------------------------------------------------
// file:    FirebaseListener.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   SSE (Server-Sent Events) listener for Firebase Realtime Database.
//          Each watch spawns a background thread that holds an SSE connection
//          open, parsing incoming events and queuing them for main-thread
//          dispatch via FirebaseManager::update().
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "FirebaseTypes.h"
#include <string>

namespace Engine
{
  namespace Firebase
  {
    namespace Listener
    {
      // Start watching an RTDB path via SSE.
      // url: full RTDB URL, e.g. "https://xxx.firebaseio.com/game_data.json"
      // on_change: called on main thread with (true, event_json) for each SSE event.
      //   event_json format: {"event":"put","path":"/doc","data":{...}}
      // Returns a unique watch_id (>= 0), or -1 on failure.
      int start_watch(const std::string& url, Callback on_change);

      // Stop a specific watch by ID. Returns true if it was found and stopped.
      bool stop_watch(int watch_id);

      // Stop all active watches (called during shutdown).
      void stop_all();

      // Drain the SSE event queue and invoke callbacks on the main thread.
      // Called from FirebaseManager::update() each frame.
      void dispatch_events();

      // Number of active watches.
      size_t active_count();

    } // namespace Listener
  } // namespace Firebase
} // namespace Engine


