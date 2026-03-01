//-----------------------------------------------------------------------------------
// file:    SkillCooldownStateCallbacks.cpp
// project: Monkey Stole My GPA
//
// brief:   State machine callbacks for per-skill cooldown UI icons (Dash, Smash,
//          Uppercut).  Each skill has its own REGISTER_STATE_MACHINE_BLUEPRINT so
//          that three independent prefab entities can be placed in the scene.
//
//          Sprite layout (2304x64, 36 frames, each 64x64):
//            Frames  0- 8  : cooldown countdown  (9=frame0 … 1=frame8)
//            Frames  9-15  : off-cooldown transition animation
//            Frames 16-35  : off-cooldown idle loop
//
//          State machine layout:
//            CD_9 … CD_1   : static frames polled every frame; transition to
//                            CD_TRANSITION when cd_timer reaches the next bracket.
//            CD_TRANSITION  : plays frames 9-15, then transitions to READY.
//            READY          : loops frames 16-35.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "../Components/GameplayComponent.h"

// ============================================================
// Helpers – read cd_timer from the player's Action for each skill
// ============================================================

static float get_dash_cd() {
  auto player = Engine::ECS::get_first_entity_with<Components::Player>();
  if (!player.is_valid()) return -1.f;
  auto* p = player.get_component<Components::Player>();
  return p ? p->dash.cd_timer : -1.f;
}

static float get_smash_cd() {
  auto player = Engine::ECS::get_first_entity_with<Components::Player>();
  if (!player.is_valid()) return -1.f;
  auto* p = player.get_component<Components::Player>();
  return p ? p->smash.cd_timer : -1.f;
}

static float get_uppercut_cd() {
  auto player = Engine::ECS::get_first_entity_with<Components::Player>();
  if (!player.is_valid()) return -1.f;
  auto* p = player.get_component<Components::Player>();
  return p ? p->uppercut.cd_timer : -1.f;
}

// ============================================================
// Generic cooldown → state name mapping.
//
// Returns the target state name for a given cd_timer value:
//   cd <= 0           → "READY"      (let transition fire from CD_1 → CD_TRANSITION → READY)
//   0  < cd <= 1      → "CD_1"
//   1  < cd <= 2      → "CD_2"
//   …
//   8  < cd           → "CD_9"  (cap at 9 for display purposes)
// ============================================================
static const char* cd_to_state(float cd) {
  if (cd <= 0.f) return "CD_TRANSITION";
  int secs = static_cast<int>(cd);          // floor
  secs = std::clamp(secs + 1, 1, 9);       // 0.x cd → show 1, 8.x cd → show 9
  static const char* names[] = {
    "CD_1", "CD_2", "CD_3", "CD_4", "CD_5",
    "CD_6", "CD_7", "CD_8", "CD_9"
  };
  return names[secs - 1];
}

// ============================================================
// Macro: update callback for a "CDN" static-frame state.
//
// Parameters:
//   skill_name   – e.g. Dash, Smash, Uppercut  (used in fn name)
//   get_cd_fn    – function returning the current cd_timer float
// ============================================================
#define DEFINE_SKILL_CD_UPDATE(skill_name, get_cd_fn)                                \
  inline void skill_name##_cd_update(Engine::ECS::Entity entity) {                   \
    float cd = get_cd_fn();                                                           \
    const char* target = cd_to_state(cd);                                            \
    auto* sm = entity.get_component<Components::StateMachine>();                     \
    if (sm && !sm->is_in_state(target)) {                                            \
      sm->change_state(target);                                                       \
    }                                                                                 \
  }

// ============================================================
// Macro: update callback for the "CD_TRANSITION" state.
// Waits for the animation to end, then switches to "READY".
// ============================================================
#define DEFINE_SKILL_TRANSITION_UPDATE(skill_name, get_cd_fn)                        \
  inline void skill_name##_transition_update(Engine::ECS::Entity entity) {           \
    /* If the cooldown restarted while in transition, go back to cooldown display */ \
    float cd = get_cd_fn();                                                           \
    if (cd > 0.f) {                                                                   \
      const char* target = cd_to_state(cd);                                          \
      auto* sm = entity.get_component<Components::StateMachine>();                   \
      if (sm) sm->change_state(target);                                               \
      return;                                                                         \
    }                                                                                 \
    auto* animation = entity.get_component<Components::Animation>();                  \
    if (animation && animation->has_ended()) {                                        \
      auto* sm = entity.get_component<Components::StateMachine>();                   \
      if (sm) sm->change_state("READY");                                              \
    }                                                                                 \
  }

// ============================================================
// Macro: update callback for the "READY" state.
// If the skill is used the cd_timer goes positive → return to CDN states.
// ============================================================
#define DEFINE_SKILL_READY_UPDATE(skill_name, get_cd_fn)                             \
  inline void skill_name##_ready_update(Engine::ECS::Entity entity) {                \
    float cd = get_cd_fn();                                                           \
    if (cd > 0.f) {                                                                   \
      const char* target = cd_to_state(cd);                                          \
      auto* sm = entity.get_component<Components::StateMachine>();                   \
      if (sm) sm->change_state(target);                                               \
    }                                                                                 \
  }

// ============================================================
// --- DASH ---
// ============================================================
DEFINE_SKILL_CD_UPDATE       (Dash, get_dash_cd)
DEFINE_SKILL_TRANSITION_UPDATE(Dash, get_dash_cd)
DEFINE_SKILL_READY_UPDATE    (Dash, get_dash_cd)

REGISTER_STATE_MACHINE_BLUEPRINT(SkillCooldown_Dash, "READY",
  STATE("CD_9",          NO_CALLBACK, Dash_cd_update,         NO_CALLBACK)
  STATE("CD_8",          NO_CALLBACK, Dash_cd_update,         NO_CALLBACK)
  STATE("CD_7",          NO_CALLBACK, Dash_cd_update,         NO_CALLBACK)
  STATE("CD_6",          NO_CALLBACK, Dash_cd_update,         NO_CALLBACK)
  STATE("CD_5",          NO_CALLBACK, Dash_cd_update,         NO_CALLBACK)
  STATE("CD_4",          NO_CALLBACK, Dash_cd_update,         NO_CALLBACK)
  STATE("CD_3",          NO_CALLBACK, Dash_cd_update,         NO_CALLBACK)
  STATE("CD_2",          NO_CALLBACK, Dash_cd_update,         NO_CALLBACK)
  STATE("CD_1",          NO_CALLBACK, Dash_cd_update,         NO_CALLBACK)
  STATE("CD_TRANSITION", NO_CALLBACK, Dash_transition_update, NO_CALLBACK)
  STATE("READY",         NO_CALLBACK, Dash_ready_update,      NO_CALLBACK)
)

// ============================================================
// --- SMASH ---
// ============================================================
DEFINE_SKILL_CD_UPDATE       (Smash, get_smash_cd)
DEFINE_SKILL_TRANSITION_UPDATE(Smash, get_smash_cd)
DEFINE_SKILL_READY_UPDATE    (Smash, get_smash_cd)

REGISTER_STATE_MACHINE_BLUEPRINT(SkillCooldown_Smash, "READY",
  STATE("CD_9",          NO_CALLBACK, Smash_cd_update,         NO_CALLBACK)
  STATE("CD_8",          NO_CALLBACK, Smash_cd_update,         NO_CALLBACK)
  STATE("CD_7",          NO_CALLBACK, Smash_cd_update,         NO_CALLBACK)
  STATE("CD_6",          NO_CALLBACK, Smash_cd_update,         NO_CALLBACK)
  STATE("CD_5",          NO_CALLBACK, Smash_cd_update,         NO_CALLBACK)
  STATE("CD_4",          NO_CALLBACK, Smash_cd_update,         NO_CALLBACK)
  STATE("CD_3",          NO_CALLBACK, Smash_cd_update,         NO_CALLBACK)
  STATE("CD_2",          NO_CALLBACK, Smash_cd_update,         NO_CALLBACK)
  STATE("CD_1",          NO_CALLBACK, Smash_cd_update,         NO_CALLBACK)
  STATE("CD_TRANSITION", NO_CALLBACK, Smash_transition_update, NO_CALLBACK)
  STATE("READY",         NO_CALLBACK, Smash_ready_update,      NO_CALLBACK)
)

// ============================================================
// --- UPPERCUT ---
// ============================================================
DEFINE_SKILL_CD_UPDATE       (Uppercut, get_uppercut_cd)
DEFINE_SKILL_TRANSITION_UPDATE(Uppercut, get_uppercut_cd)
DEFINE_SKILL_READY_UPDATE    (Uppercut, get_uppercut_cd)

REGISTER_STATE_MACHINE_BLUEPRINT(SkillCooldown_Uppercut, "READY",
  STATE("CD_9",          NO_CALLBACK, Uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_8",          NO_CALLBACK, Uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_7",          NO_CALLBACK, Uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_6",          NO_CALLBACK, Uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_5",          NO_CALLBACK, Uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_4",          NO_CALLBACK, Uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_3",          NO_CALLBACK, Uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_2",          NO_CALLBACK, Uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_1",          NO_CALLBACK, Uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_TRANSITION", NO_CALLBACK, Uppercut_transition_update, NO_CALLBACK)
  STATE("READY",         NO_CALLBACK, Uppercut_ready_update,      NO_CALLBACK)
)
