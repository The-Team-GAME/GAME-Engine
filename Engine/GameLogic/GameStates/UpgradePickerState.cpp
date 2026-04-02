//-----------------------------------------------------------------------------------
// file:    UpgradePickerState.cpp
// project: Monkey Stole My GPA
//
// brief:   See UpgradePickerState.h
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "UpgradePickerState.h"
#include "GameplayOverlayInputGuard.h"
#include "../../Engine/UpgradeUIStateMachine.h"

void UpgradePickerState::load()
{
  GameplayOverlayInputGuard::acquire();
}
void UpgradePickerState::init()   {}
void UpgradePickerState::draw()   {}
void UpgradePickerState::free()   {}
void UpgradePickerState::unload()
{
  GameplayOverlayInputGuard::release();
}

void UpgradePickerState::update()
{
  // Drive all picker input / animation.
  Engine::UpgradeUIStateMachine::instance().update();

  // Pop ourselves once the FSM has finished picking (Applying/Skipping are
  // transient — after one update() call they land back in Idle or Available).
  auto s = Engine::UpgradeUIStateMachine::instance().get_state();
  if (s != Engine::UpgradeFlowState::PickerOpen &&
      s != Engine::UpgradeFlowState::Rolling    &&
      s != Engine::UpgradeFlowState::Applying   &&
      s != Engine::UpgradeFlowState::Skipping)
  {
    Engine::SceneManager::pop_scene();
  }
}
