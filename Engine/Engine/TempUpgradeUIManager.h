//-----------------------------------------------------------------------------------
// file:    TempUpgradeUIManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ENGINE_API bridge so the GameLogic DLL can trigger the in-game upgrade
//          picker UI without calling ImGui directly. All ImGui work is executed
//          on the Engine EXE side where the ImGui context is always valid.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "EngineExport.h"

namespace Engine
{
  class TempUpgradeUIManager
  {
  public:
    // Call from GameplayState::draw() (in the GameLogic DLL).
    // Internally forwards to UpgradePickerUI() which runs on the Engine EXE side.
    ENGINE_API static void draw();
  };
}


