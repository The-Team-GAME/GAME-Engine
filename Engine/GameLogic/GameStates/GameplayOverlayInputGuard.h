//-----------------------------------------------------------------------------------
// file:    GameplayOverlayInputGuard.h
// project: Monkey Stole My GPA
//
// brief:   Shared helper for pushed overlay scenes to lock player gameplay input
//          while UI overlays are active and to swallow held inputs on resume.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"
#include "../Components/GameplayComponent.h"
#include "../../Engine/InputManager.h"

namespace GameplayOverlayInputGuard
{
  inline int active_overlay_count = 0;
  inline bool restore_player_input_on_release = false;

  inline Components::Player* try_get_player_component()
  {
    Engine::ECS::Entity player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    if (!player_entity.is_valid()) {
      return nullptr;
    }

    return player_entity.get_component<Components::Player>();
  }

  inline void acquire()
  {
    if (active_overlay_count++ > 0) {
      return;
    }

    if (Components::Player* player_comp = try_get_player_component()) {
      restore_player_input_on_release = !player_comp->input_lock;
      player_comp->input_lock = true;
    }
    else {
      restore_player_input_on_release = false;
    }
  }

  inline void release()
  {
    if (active_overlay_count <= 0) {
      active_overlay_count = 0;
      restore_player_input_on_release = false;
      return;
    }

    if (--active_overlay_count > 0) {
      return;
    }

    Engine::Input::suppress_current_inputs_until_release();

    if (restore_player_input_on_release) {
      if (Components::Player* player_comp = try_get_player_component()) {
        player_comp->input_lock = false;
      }
    }

    restore_player_input_on_release = false;
  }
}