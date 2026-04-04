//-----------------------------------------------------------------------------------
// file:    TempUpgradeUIManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Legacy no-op implementation kept for upgrade UI ABI compatibility.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "TempUpgradeUIManager.h"
#include "Precompiled.h"

namespace Engine
{
  void TempUpgradeUIManager::draw()
  {
    // Upgrade picker UI is now handled by UpgradeUIStateMachine (entity-based).
    // This function is kept as a no-op for ABI compatibility.
  }
}


