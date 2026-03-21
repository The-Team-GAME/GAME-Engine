//-----------------------------------------------------------------------------------
// file:    UpgradePickerState.h
// project: Monkey Stole My GPA
//
// brief:   Overlay scene pushed by UpgradeUIStateMachine when the card picker
//          opens. Owns the per-frame FSM update so no other scene ever sees input
//          while the picker is active. When the FSM transitions back to Idle or
//          Available the state pops itself off the stack.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class UpgradePickerState : public Engine::Scene
{
public:
  void load()   override;
  void init()   override;
  void update() override;
  void draw()   override;
  void free()   override;
  void unload() override;
};
