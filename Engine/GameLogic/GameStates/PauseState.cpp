//-----------------------------------------------------------------------------------
// file:    PauseState.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation details for the pause overlay scene and its audio snapshot helpers.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "PauseState.h"
#include "../../Engine/Engine/Audio/AudioSystem.h"

namespace {
  static constexpr const char* PAUSE_LOWPASS_SNAPSHOT_INSTANCE = "pause_lowpass_snapshot";
  static constexpr const char* PAUSE_LOWPASS_SNAPSHOT_EVENT = "snapshot:/LowpassSnapshot";

  static bool gPauseLowpassSnapshotActive = false;
}

namespace PauseStateDetail {

  void start_pause_lowpass_snapshot() {
    if (gPauseLowpassSnapshotActive) return;

    if (!Engine::AudioSystem::createEventInstance(PAUSE_LOWPASS_SNAPSHOT_INSTANCE, PAUSE_LOWPASS_SNAPSHOT_EVENT)) {
      return;
    }

    if (!Engine::AudioSystem::startEventInstance(PAUSE_LOWPASS_SNAPSHOT_INSTANCE)) {
      Engine::AudioSystem::releaseEventInstance(PAUSE_LOWPASS_SNAPSHOT_INSTANCE);
      return;
    }

    gPauseLowpassSnapshotActive = true;
  }

  void stop_pause_lowpass_snapshot() {
    if (!gPauseLowpassSnapshotActive) return;

    Engine::AudioSystem::stopEventInstance(PAUSE_LOWPASS_SNAPSHOT_INSTANCE, false);
    Engine::AudioSystem::releaseEventInstance(PAUSE_LOWPASS_SNAPSHOT_INSTANCE);
    gPauseLowpassSnapshotActive = false;
  }
}