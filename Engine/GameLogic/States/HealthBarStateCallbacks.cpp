#include "pch.h"
#include "../Components/GameplayComponent.h"
#include "../Components/HealthComponents.h"

// ========================================================================
// Helper: get current player health value
// ========================================================================
static int get_player_health() {
  auto player = Engine::ECS::get_first_entity_with<Components::Player>();
  if (!player.is_valid()) return -1;
  auto* hp = player.get_component<Components::Health>();
  return hp ? hp->value : -1;
}

// ========================================================================
// Static HP state update — check if player health dropped, start transition
// ========================================================================

// Macro to generate the update callback for a static HP state.
// `hp_val`       = the health value this state represents
// `trans_state`  = the transition state to go to when health drops
#define DEFINE_HP_STATE_UPDATE(hp_val, trans_state)                          \
  inline void hp_##hp_val##_update(Engine::ECS::Entity entity) {            \
    int health = get_player_health();                                       \
    if (health < 0) return;                                                 \
    if (health < hp_val) {                                                  \
      auto* sm = entity.get_component<Components::StateMachine>();          \
      if (sm) sm->change_state(trans_state);                                \
    }                                                                       \
  }

DEFINE_HP_STATE_UPDATE(5, "5HP_to_4HP")
DEFINE_HP_STATE_UPDATE(4, "4HP_to_3HP")
DEFINE_HP_STATE_UPDATE(3, "3HP_to_2HP")
DEFINE_HP_STATE_UPDATE(2, "2HP_to_1HP")
DEFINE_HP_STATE_UPDATE(1, "1HP_to_0HP")

// 0HP is the terminal state — no further transitions
inline void hp_0_update([[maybe_unused]] Engine::ECS::Entity entity) {}

// ========================================================================
// Transition state update — wait for animation to end, then go to next state
// ========================================================================

// Macro to generate the update callback for a transition state.
// `target_state` = the static state to transition to when animation ends
#define DEFINE_TRANSITION_UPDATE(from, to, target_state)                    \
  inline void hp_##from##_to_##to##_update(Engine::ECS::Entity entity) {   \
    auto* animation = entity.get_component<Components::Animation>();        \
    if (animation && animation->has_ended()) {                              \
      auto* sm = entity.get_component<Components::StateMachine>();          \
      if (sm) sm->change_state(target_state);                               \
    }                                                                       \
  }

DEFINE_TRANSITION_UPDATE(5, 4, "4HP")
DEFINE_TRANSITION_UPDATE(4, 3, "3HP")
DEFINE_TRANSITION_UPDATE(3, 2, "2HP")
DEFINE_TRANSITION_UPDATE(2, 1, "1HP")
DEFINE_TRANSITION_UPDATE(1, 0, "0HP")

// ========================================================================
// Blueprint Registration
// ========================================================================

REGISTER_STATE_MACHINE_BLUEPRINT(HealthBar, "5HP",
  STATE("5HP", NO_CALLBACK, hp_5_update, NO_CALLBACK)
  STATE("5HP_to_4HP", NO_CALLBACK, hp_5_to_4_update, NO_CALLBACK)
  STATE("4HP", NO_CALLBACK, hp_4_update, NO_CALLBACK)
  STATE("4HP_to_3HP", NO_CALLBACK, hp_4_to_3_update, NO_CALLBACK)
  STATE("3HP", NO_CALLBACK, hp_3_update, NO_CALLBACK)
  STATE("3HP_to_2HP", NO_CALLBACK, hp_3_to_2_update, NO_CALLBACK)
  STATE("2HP", NO_CALLBACK, hp_2_update, NO_CALLBACK)
  STATE("2HP_to_1HP", NO_CALLBACK, hp_2_to_1_update, NO_CALLBACK)
  STATE("1HP", NO_CALLBACK, hp_1_update, NO_CALLBACK)
  STATE("1HP_to_0HP", NO_CALLBACK, hp_1_to_0_update, NO_CALLBACK)
  STATE("0HP", NO_CALLBACK, hp_0_update, NO_CALLBACK)
)
