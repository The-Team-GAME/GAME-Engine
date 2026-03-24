
#include "pch.h"
#include "../../Engine/ECS/ECS.h"
#include "../../Engine/InputManager.h"
#include "../../Engine/Logger.h"
#include "../../Engine/Resources.h"
#include "../../Engine/SceneManager.h"
#include "../../Engine/Time.h"
#include "../../Engine/Systems/GraphicsSystem.h"
#include "../GameStates/OldManDialogueState.h"

namespace OldManCallbacks
{
  inline bool e_key_was_down = false;  // Track previous E key state for edge detection

  // Press E prompt entity shown above the oldman while in proximity
  inline Engine::ECS::EntityID press_e_prompt_id = 0;
  static constexpr float PRESS_E_FADE_SPEED = 2.5f;  // Alpha units per second
  static constexpr float PRESS_E_PROMPT_OFFSET_Y = -90.0f; // Pixels above oldman centre

  void spawn_press_e_prompt(Engine::ECS::Entity oldman_entity) {
    if (press_e_prompt_id != 0) return;

    auto* oldman_tf = oldman_entity.get_component<Components::Transform2D>();
    if (!oldman_tf) return;

    float prompt_x = oldman_tf->position.x;
    float prompt_y = oldman_tf->position.y + PRESS_E_PROMPT_OFFSET_Y;

    // PressE.png is 1152x64 with 9 frames (128x64 each, single row)
    // Pre-construct atlas and animation to avoid is_constructible_v failures on mixed int/size_t args
    Components::TextureAtlas press_e_atlas(128, 64, (size_t)1, (size_t)9, (size_t)9);
    Components::Animation press_e_anim((size_t)9, 0.1f, true);
    auto prompt = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(prompt_x, prompt_y, 0.0f, 128.0f * 2.0f, 64.0f * 2.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/PressE.png"))
      .add_component<Components::TextureAtlas>(std::move(press_e_atlas))
      .add_component<Components::Animation>(std::move(press_e_anim))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)  // start fully transparent
      .add_to_layer("player");

    press_e_prompt_id = prompt.get_id();
  }

  void destroy_press_e_prompt() {
    if (press_e_prompt_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(press_e_prompt_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(press_e_prompt_id));
    }
    press_e_prompt_id = 0;
  }

  void spawn_dialogue(Engine::ECS::Entity) {
    if (OldManDialogueState::is_open) return;

    // Hide the press E prompt while dialogue is up
    destroy_press_e_prompt();

    Engine::SceneManager::push_scene("OldManDialogueState");

    Log::info("Old man dialogue spawned!");
  }

  void destroy_dialogue() {
    if (!OldManDialogueState::is_open) return;

    Engine::SceneManager::pop_scene();

    Log::info("Old man dialogue destroyed!");
  }

  void old_man_on_enter(Engine::ECS::Entity oldman_entity, Engine::ECS::Entity player_entity) {
    Log::info("Player {} entered interaction range", player_entity.get_id());
    spawn_press_e_prompt(oldman_entity);
  }

  void old_man_on_update(Engine::ECS::Entity oldman_entity, Engine::ECS::Entity player_entity) {
    if (OldManDialogueState::is_open) {
      destroy_press_e_prompt();
      e_key_was_down = Engine::Input::is_held(Engine::KeyCode::E);
      return;
    }

    // Fade in the press E prompt using unscaled dt (works even when time scale is near-zero)
    if (press_e_prompt_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(press_e_prompt_id))) {
      auto* color = Engine::ECS::Entity(press_e_prompt_id).get_component<Components::Color>();
      if (color && color->a < 1.0f) {
        float dt = Engine::Time::get_unscaled_fixed_dt();
        color->a += PRESS_E_FADE_SPEED * dt;
        if (color->a > 1.0f) color->a = 1.0f;
      }

      // Keep the prompt anchored above the oldman as it moves
      auto* prompt_tf = Engine::ECS::Entity(press_e_prompt_id).get_component<Components::Transform2D>();
      auto* oldman_tf = oldman_entity.get_component<Components::Transform2D>();
      if (prompt_tf && oldman_tf) {
        prompt_tf->position.x = oldman_tf->position.x;
        prompt_tf->position.y = oldman_tf->position.y + PRESS_E_PROMPT_OFFSET_Y;
      }
    }

    // Manual edge detection for E key (since time scale affects is_pressed behavior)
    bool e_key_down = Engine::Input::is_held(Engine::KeyCode::E);

    // Only trigger on the rising edge (key just went from up to down)
    if (e_key_down && !e_key_was_down) {
      spawn_dialogue(player_entity);
    }

    e_key_was_down = e_key_down;
  }

  void old_man_on_exit(Engine::ECS::Entity interactable, Engine::ECS::Entity player_entity) {
    Log::info("Player {} exited interaction range of entity {}", player_entity.get_id(), interactable.get_id());
    destroy_press_e_prompt();
    destroy_dialogue();
  }
} // namespace OldManCallbacks


REGISTER_FUNCTION(OldManCallbacks::old_man_on_enter)
REGISTER_FUNCTION(OldManCallbacks::old_man_on_update)
REGISTER_FUNCTION(OldManCallbacks::old_man_on_exit)