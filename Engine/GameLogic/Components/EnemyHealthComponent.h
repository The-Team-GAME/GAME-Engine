//-----------------------------------------------------------------------------------
// file:    EnemyHealthComponent.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Component for enemy health management with state machine integration
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"

namespace Components {

  struct EnemyHealth {
    int current_hp = 100;
    int max_hp = 100;

    void take_damage(int amount) {
      current_hp = std::max(0, current_hp - amount);
    }

    void heal(int amount) {
      current_hp = std::min(max_hp, current_hp + amount);
    }

    bool is_dead() const {
      return current_hp <= 0;
    }
  };

}

REGISTER_COMPONENT(Components::EnemyHealth, current_hp, max_hp)
