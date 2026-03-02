#include "pch.h"
#include "../Components/GameplayComponent.h"

// ========================================================================
// Sprite sheet layout (2304x64, 36 frames, 64x64 each):
//   Frame 0..8   = CD countdown (frame 0 = "9s", frame 8 = "1s")
//   Frame 9..34  = Transition animation (skill coming off cooldown)
//   Frame 35     = READY idle
// ========================================================================

// ========================================================================
// Helper: retrieve the player component
// ========================================================================
static Components::Player* get_player()
{
  auto entity = Engine::ECS::get_first_entity_with<Components::Player>();
  return entity.is_valid() ? entity.get_component<Components::Player>() : nullptr;
}

// ========================================================================
// Helper: map a cd_timer float -> countdown state name.
//   "Round-up" logic: 0.5s remaining shows "CD_1", not 0.
//   Returns "CD_TRANSITION" when cd <= 0 (timer finished).
// ========================================================================
static const char* cd_to_state(float cd)
{
  if (cd <= 0.f) return "CD_TRANSITION";

  int secs = static_cast<int>(cd);       // floor
  secs = std::clamp(secs + 1, 1, 9);    // 0.x -> 1,  8.x/9.x -> 9

  static const char* names[] = {
    "CD_1", "CD_2", "CD_3", "CD_4", "CD_5",
    "CD_6", "CD_7", "CD_8", "CD_9"
  };
  return names[secs - 1];
}

// ========================================================================
// Transition state update — wait for animation end, then go to READY
// ========================================================================
static void cd_transition_update(Engine::ECS::Entity entity)
{
  auto* animation = entity.get_component<Components::Animation>();
  if (animation && animation->has_ended())
  {
    auto* sm = entity.get_component<Components::StateMachine>();
    if (sm) sm->change_state("READY");
  }
}

// ========================================================================
// Macro: countdown update — used by CD_9..CD_1.
//   Polls every frame; when cd reaches 0, drives state to CD_TRANSITION.
// ========================================================================
#define DEFINE_CD_UPDATE(skill_name, get_cd_expr)                                  \
  static void skill_name##_cd_update(Engine::ECS::Entity entity)                   \
  {                                                                                 \
    auto* p = get_player();                                                         \
    if (!p) return;                                                                 \
    float cd = (get_cd_expr);                                                       \
    const char* target = cd_to_state(cd);                                          \
    auto* sm = entity.get_component<Components::StateMachine>();                    \
    if (sm && !sm->is_in_state(target))                                             \
      sm->change_state(target);                                                     \
  }

// ========================================================================
// Macro: READY state update — used ONLY by the READY state.
//   cd_timer == 0  ->  skill is available, stay in READY (early return).
//   cd_timer >  0  ->  skill just used, switch to countdown state.
// ========================================================================
#define DEFINE_READY_UPDATE(skill_name, get_cd_expr)                               \
  static void skill_name##_ready_update(Engine::ECS::Entity entity)                \
  {                                                                                 \
    auto* p = get_player();                                                         \
    if (!p) return;                                                                 \
    float cd = (get_cd_expr);                                                       \
    if (cd <= 0.f) return;          /* skill available — stay in READY */          \
    const char* target = cd_to_state(cd);                                          \
    auto* sm = entity.get_component<Components::StateMachine>();                    \
    if (sm && !sm->is_in_state(target))                                             \
      sm->change_state(target);                                                     \
  }

// ========================================================================
// Macro: per-skill transition update (delegates to shared helper)
// ========================================================================
#define DEFINE_TRANSITION_UPDATE(skill_name)                                       \
  static void skill_name##_transition_update(Engine::ECS::Entity entity)           \
  {                                                                                 \
    cd_transition_update(entity);                                                   \
  }

DEFINE_CD_UPDATE(dash,     p->dash.cd_timer)
DEFINE_CD_UPDATE(smash,    p->smash.cd_timer)
DEFINE_CD_UPDATE(uppercut, p->uppercut.cd_timer)

DEFINE_READY_UPDATE(dash,     p->dash.cd_timer)
DEFINE_READY_UPDATE(smash,    p->smash.cd_timer)
DEFINE_READY_UPDATE(uppercut, p->uppercut.cd_timer)

DEFINE_TRANSITION_UPDATE(dash)
DEFINE_TRANSITION_UPDATE(smash)
DEFINE_TRANSITION_UPDATE(uppercut)

// ========================================================================
// Blueprint Registration
// ========================================================================

REGISTER_STATE_MACHINE_BLUEPRINT(DashCooldown, "READY",
  STATE("CD_9",         NO_CALLBACK, dash_cd_update,         NO_CALLBACK)
  STATE("CD_8",         NO_CALLBACK, dash_cd_update,         NO_CALLBACK)
  STATE("CD_7",         NO_CALLBACK, dash_cd_update,         NO_CALLBACK)
  STATE("CD_6",         NO_CALLBACK, dash_cd_update,         NO_CALLBACK)
  STATE("CD_5",         NO_CALLBACK, dash_cd_update,         NO_CALLBACK)
  STATE("CD_4",         NO_CALLBACK, dash_cd_update,         NO_CALLBACK)
  STATE("CD_3",         NO_CALLBACK, dash_cd_update,         NO_CALLBACK)
  STATE("CD_2",         NO_CALLBACK, dash_cd_update,         NO_CALLBACK)
  STATE("CD_1",         NO_CALLBACK, dash_cd_update,         NO_CALLBACK)
  STATE("CD_TRANSITION",NO_CALLBACK, dash_transition_update, NO_CALLBACK)
  STATE("READY",        NO_CALLBACK, dash_ready_update,      NO_CALLBACK)
)

REGISTER_STATE_MACHINE_BLUEPRINT(SmashCooldown, "READY",
  STATE("CD_9",         NO_CALLBACK, smash_cd_update,         NO_CALLBACK)
  STATE("CD_8",         NO_CALLBACK, smash_cd_update,         NO_CALLBACK)
  STATE("CD_7",         NO_CALLBACK, smash_cd_update,         NO_CALLBACK)
  STATE("CD_6",         NO_CALLBACK, smash_cd_update,         NO_CALLBACK)
  STATE("CD_5",         NO_CALLBACK, smash_cd_update,         NO_CALLBACK)
  STATE("CD_4",         NO_CALLBACK, smash_cd_update,         NO_CALLBACK)
  STATE("CD_3",         NO_CALLBACK, smash_cd_update,         NO_CALLBACK)
  STATE("CD_2",         NO_CALLBACK, smash_cd_update,         NO_CALLBACK)
  STATE("CD_1",         NO_CALLBACK, smash_cd_update,         NO_CALLBACK)
  STATE("CD_TRANSITION",NO_CALLBACK, smash_transition_update, NO_CALLBACK)
  STATE("READY",        NO_CALLBACK, smash_ready_update,      NO_CALLBACK)
)

REGISTER_STATE_MACHINE_BLUEPRINT(UppercutCooldown, "READY",
  STATE("CD_9",         NO_CALLBACK, uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_8",         NO_CALLBACK, uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_7",         NO_CALLBACK, uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_6",         NO_CALLBACK, uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_5",         NO_CALLBACK, uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_4",         NO_CALLBACK, uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_3",         NO_CALLBACK, uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_2",         NO_CALLBACK, uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_1",         NO_CALLBACK, uppercut_cd_update,         NO_CALLBACK)
  STATE("CD_TRANSITION",NO_CALLBACK, uppercut_transition_update, NO_CALLBACK)
  STATE("READY",        NO_CALLBACK, uppercut_ready_update,      NO_CALLBACK)
)
