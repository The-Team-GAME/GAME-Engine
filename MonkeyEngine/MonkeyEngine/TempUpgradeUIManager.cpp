//-----------------------------------------------------------------------------------
// file:    TempUpgradeUIManager.cpp
// project: Monkey Stole My GPA
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
