//-----------------------------------------------------------------------------------
// file:    SkillCooldownStateCallbacks.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   State machine callbacks that map player skill cooldown timers to UI states.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "../Components/GameplayComponent.h"

// ========================================================================
// Sprite sheet layout (2304x64, 36 frames, 64x64 each):
//   Frame 0..8   = CD countdown (frame 0 = "9s", frame 8 = "1s")
//   Frame 9..34  = Transition animation (0 < cd < 1s)
//   Frame 35     = READY (cd <= 0)
// ========================================================================

static Components::Player* get_player()
{
  auto entity = Engine::ECS::get_first_entity_with<Components::Player>();
  return entity.is_valid() ? entity.get_component<Components::Player>() : nullptr;
}

// ========================================================================
// Value-driven state mapping — called every frame for every state.
//   cd <= 0          ->  READY
//   0 < cd < 1       ->  CD_TRANSITION
//   cd >= 1          ->  CD_1 .. CD_9  (floor, clamped)
// ========================================================================
static const char* cd_to_state(float cd)
{
  if (cd <= 0.f) return "READY";
  if (cd <  1.f) return "CD_TRANSITION";

  int secs = std::clamp(static_cast<int>(cd), 1, 9);
  static const char* names[] = {
    "CD_1", "CD_2", "CD_3", "CD_4", "CD_5",
    "CD_6", "CD_7", "CD_8", "CD_9"
  };
  return names[secs - 1];
}

// ========================================================================
// One update callback per skill — drives the state machine purely from
// the cd_timer value, switching states whenever the bucket changes.
// ========================================================================
#define DEFINE_UPDATE(skill_name, is_ready_expr, get_cd_expr)                    \
  static void skill_name##_update(Engine::ECS::Entity entity)                    \
  {                                                                               \
    auto* p = get_player();                                                       \
    if (!p) return;                                                               \
    float cd = (is_ready_expr) ? 0.f : (get_cd_expr);                             \
    const char* target = cd_to_state(cd);                                        \
    auto* sm = entity.get_component<Components::StateMachine>();                  \
    if (sm && !sm->is_in_state(target))                                           \
      sm->change_state(target);                                                   \
  }

DEFINE_UPDATE(dash,     p->dash_ready(),     p->dash.cd_timer)
DEFINE_UPDATE(smash,    p->smash_ready(),    p->smash.cd_timer)
DEFINE_UPDATE(uppercut, p->uppercut_ready(), p->uppercut.cd_timer)

// ========================================================================
// Blueprint Registration
// ========================================================================

REGISTER_STATE_MACHINE_BLUEPRINT(DashCooldown, "READY",
  STATE("CD_9",         NO_CALLBACK, dash_update, NO_CALLBACK)
  STATE("CD_8",         NO_CALLBACK, dash_update, NO_CALLBACK)
  STATE("CD_7",         NO_CALLBACK, dash_update, NO_CALLBACK)
  STATE("CD_6",         NO_CALLBACK, dash_update, NO_CALLBACK)
  STATE("CD_5",         NO_CALLBACK, dash_update, NO_CALLBACK)
  STATE("CD_4",         NO_CALLBACK, dash_update, NO_CALLBACK)
  STATE("CD_3",         NO_CALLBACK, dash_update, NO_CALLBACK)
  STATE("CD_2",         NO_CALLBACK, dash_update, NO_CALLBACK)
  STATE("CD_1",         NO_CALLBACK, dash_update, NO_CALLBACK)
  STATE("CD_TRANSITION",NO_CALLBACK, dash_update, NO_CALLBACK)
  STATE("READY",        NO_CALLBACK, dash_update, NO_CALLBACK)
)

REGISTER_STATE_MACHINE_BLUEPRINT(SmashCooldown, "READY",
  STATE("CD_9",         NO_CALLBACK, smash_update, NO_CALLBACK)
  STATE("CD_8",         NO_CALLBACK, smash_update, NO_CALLBACK)
  STATE("CD_7",         NO_CALLBACK, smash_update, NO_CALLBACK)
  STATE("CD_6",         NO_CALLBACK, smash_update, NO_CALLBACK)
  STATE("CD_5",         NO_CALLBACK, smash_update, NO_CALLBACK)
  STATE("CD_4",         NO_CALLBACK, smash_update, NO_CALLBACK)
  STATE("CD_3",         NO_CALLBACK, smash_update, NO_CALLBACK)
  STATE("CD_2",         NO_CALLBACK, smash_update, NO_CALLBACK)
  STATE("CD_1",         NO_CALLBACK, smash_update, NO_CALLBACK)
  STATE("CD_TRANSITION",NO_CALLBACK, smash_update, NO_CALLBACK)
  STATE("READY",        NO_CALLBACK, smash_update, NO_CALLBACK)
)

REGISTER_STATE_MACHINE_BLUEPRINT(UppercutCooldown, "READY",
  STATE("CD_9",         NO_CALLBACK, uppercut_update, NO_CALLBACK)
  STATE("CD_8",         NO_CALLBACK, uppercut_update, NO_CALLBACK)
  STATE("CD_7",         NO_CALLBACK, uppercut_update, NO_CALLBACK)
  STATE("CD_6",         NO_CALLBACK, uppercut_update, NO_CALLBACK)
  STATE("CD_5",         NO_CALLBACK, uppercut_update, NO_CALLBACK)
  STATE("CD_4",         NO_CALLBACK, uppercut_update, NO_CALLBACK)
  STATE("CD_3",         NO_CALLBACK, uppercut_update, NO_CALLBACK)
  STATE("CD_2",         NO_CALLBACK, uppercut_update, NO_CALLBACK)
  STATE("CD_1",         NO_CALLBACK, uppercut_update, NO_CALLBACK)
  STATE("CD_TRANSITION",NO_CALLBACK, uppercut_update, NO_CALLBACK)
  STATE("READY",        NO_CALLBACK, uppercut_update, NO_CALLBACK)
)


