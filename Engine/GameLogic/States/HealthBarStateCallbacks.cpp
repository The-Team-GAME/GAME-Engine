#include "pch.h"

inline void temp([[maybe_unused]] Engine::ECS::Entity entity) {
  Log::info("Health changed");
}


REGISTER_STATE_MACHINE_BLUEPRINT(HealthBar, "Idle",
  STATE("5HP", temp, NO_CALLBACK, NO_CALLBACK)
  STATE("5HP_to_4HP", temp, NO_CALLBACK, NO_CALLBACK)
  STATE("4HP", temp, NO_CALLBACK, NO_CALLBACK)
  STATE("4HP_to_3HP", temp, NO_CALLBACK, NO_CALLBACK)
  STATE("3HP", temp, NO_CALLBACK, NO_CALLBACK)
  STATE("3HP_to_2HP", temp, NO_CALLBACK, NO_CALLBACK)
  STATE("2HP", temp, NO_CALLBACK, NO_CALLBACK)
  STATE("2HP_to_1HP", temp, NO_CALLBACK, NO_CALLBACK)
  STATE("1HP", temp, NO_CALLBACK, NO_CALLBACK)
  STATE("1HP_to_0HP", temp, NO_CALLBACK, NO_CALLBACK)
)
