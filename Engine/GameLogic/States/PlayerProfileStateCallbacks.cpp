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
// Per-HP state update — switch directly when health changes
// ========================================================================
static const char* hp_to_state[] = { "0HP", "1HP", "2HP", "3HP", "4HP", "5HP" };

inline void profile_hp_update(Engine::ECS::Entity entity) {

  return; // TEMP: Disable for now

  int health = get_player_health();
  if (health < 0) return;
  health = std::clamp(health, 0, 5);

  auto* sm = entity.get_component<Components::StateMachine>();
  if (!sm) return;

  const char* target = hp_to_state[health];
  if (!sm->is_in_state(target)) {
    sm->change_state(target);
  }
}

// ========================================================================
// Blueprint Registration
// ========================================================================

REGISTER_STATE_MACHINE_BLUEPRINT(PlayerProfile, "5HP",
  STATE("5HP", NO_CALLBACK, profile_hp_update, NO_CALLBACK)
  STATE("4HP", NO_CALLBACK, profile_hp_update, NO_CALLBACK)
  STATE("3HP", NO_CALLBACK, profile_hp_update, NO_CALLBACK)
  STATE("2HP", NO_CALLBACK, profile_hp_update, NO_CALLBACK)
  STATE("1HP", NO_CALLBACK, profile_hp_update, NO_CALLBACK)
  STATE("0HP", NO_CALLBACK, profile_hp_update, NO_CALLBACK)
)
