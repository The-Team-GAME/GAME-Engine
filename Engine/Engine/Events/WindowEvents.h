//-----------------------------------------------------------------------------------
// file:    BasicEvents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines event structures used throughout the game engine.
//          Includes window events (WindowResizeEvent), and file handling events
//          (FileDropEvent).
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "../TypeDef.h"
#include <string>
#include <vector>

namespace Engine {
  namespace Events {

    /* ================= //
    // Window events     //
    // ================= */
    struct WindowResizeEvent {
      s32 width;
      s32 height;
    };

    /* ====================== //
    // File handling events   //
    // ====================== */
    struct FileDropEvent {
      std::vector<std::string> filepaths;
    };

  } // namespace Events
} // namespace Engine