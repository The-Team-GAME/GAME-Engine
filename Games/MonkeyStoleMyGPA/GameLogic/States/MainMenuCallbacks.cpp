#include "pch.h"

namespace MainMenu
{
  inline void sleeping_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    if (!state_machine) return;

    // Log::info("MainMenuState sleeping_on_update: waiting for key press to transition to Menu state");

    for (int key = 32; key <= 348; ++key) { // GLFW_KEY_SPACE to GLFW_KEY_LAST
      if (Engine::Input::is_pressed(static_cast<Engine::KeyCode>(key))) {
        state_machine->change_state("Menu");
      }
      // Log::info("MainMenuState sleeping_on_update: checking key {}", key);
    }

  }
}

REGISTER_STATE_MACHINE_BLUEPRINT(MainMenu, "Start",
  STATE("Start", NO_CALLBACK, MainMenu::sleeping_on_update, NO_CALLBACK)
  STATE("Menu", NO_CALLBACK, NO_CALLBACK, NO_CALLBACK)
)
