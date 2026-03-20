#pragma once

#include "pch.h"
#include "../../Engine/Systems/GraphicsSystem.h"

namespace OldManCallbacks
{
  // Static entity IDs for dialogue UI (to track and clean up)
  inline ECS::EntityID dialogue_bg_id = 0;
  inline ECS::EntityID npc_portrait_id = 0;
  inline ECS::EntityID dialogue_text_id = 0;
  inline bool dialogue_shown = false;
  inline bool e_key_was_down = false;  // Track previous E key state for edge detection
  inline bool tips_overlay_request_pending = false;

  // Press E prompt entity shown above the oldman while in proximity
  inline ECS::EntityID press_e_prompt_id = 0;
  static constexpr float PRESS_E_FADE_SPEED = 2.5f;  // Alpha units per second
  static constexpr float PRESS_E_PROMPT_OFFSET_Y = -90.0f; // Pixels above oldman centre

  bool consume_tips_overlay_request();
  void trigger_tips_overlay_from_gameplay();

  void spawn_press_e_prompt(ECS::Entity oldman_entity) {
    if (press_e_prompt_id != 0) return;

    auto* oldman_tf = oldman_entity.get_component<Components::Transform2D>();
    if (!oldman_tf) return;

    float prompt_x = oldman_tf->position.x;
    float prompt_y = oldman_tf->position.y + PRESS_E_PROMPT_OFFSET_Y;

    // PressE.png is 1152x64 with 9 frames (128x64 each, single row)
    // Pre-construct atlas and animation to avoid is_constructible_v failures on mixed int/size_t args
    Components::TextureAtlas press_e_atlas(128, 64, (size_t)1, (size_t)9, (size_t)9);
    Components::Animation press_e_anim((size_t)9, 0.1f, true);
    auto prompt = ECS::create_entity()
      .add_component<Components::Transform2D>(prompt_x, prompt_y, 0.0f, 128.0f * 2.0f, 64.0f * 2.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/PressE.png"))
      .add_component<Components::TextureAtlas>(std::move(press_e_atlas))
      .add_component<Components::Animation>(std::move(press_e_anim))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)  // start fully transparent
      .add_to_layer("player");

    press_e_prompt_id = prompt.get_id();
  }

  void destroy_press_e_prompt() {
    if (press_e_prompt_id != 0 && ECS::is_entity_active(ECS::Entity(press_e_prompt_id))) {
      ECS::destroy_entity(ECS::Entity(press_e_prompt_id));
    }
    press_e_prompt_id = 0;
  }

  void spawn_dialogue(ECS::Entity) {
    if (dialogue_shown) return;
    dialogue_shown = true;

    // Hide the press E prompt while dialogue is up
    destroy_press_e_prompt();

    // Get camera position to center dialogue on screen
    Components::Transform2D camera = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float cam_x = camera.position.x + 800.0f;  // Screen center X
    float cam_y = camera.position.y + 450.0f;  // Screen center Y

    // Create dialogue background - covers whole screen
    auto dialogue_bg = ECS::create_entity()
      .add_component<Components::Transform2D>(cam_x, cam_y, 0.0f, 1600.0f, 900.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/NotBroDialogue.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_4");
    dialogue_bg_id = dialogue_bg.get_id();

    // Create NPC portrait (NPCOldman.png) - bottom right of screen
    auto npc_portrait = ECS::create_entity()
      .add_component<Components::Transform2D>(cam_x + 509.0f, cam_y + 173.0f, 0.0f, 320.0f, 320.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/NPCOldman.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_5");
    npc_portrait_id = npc_portrait.get_id();

    // Create dialogue text - bottom of screen
    auto dialogue_text = ECS::create_entity()
      .add_component<Components::Transform2D>(cam_x - 177.0f, cam_y + 270.0f, 0.0f, 631.0f, 166.0f)
      .add_component<Components::Font>(Engine::Resources::load_font("fonts/FiveFontsatFreddy's-Regular.ttf", 48))
      .add_component<Components::Text>("Its time to retrieve your homework boy! Reach the end of the station and take down the big monkey FOR YOUR GPA!", 0.5f)
      .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_5");
    dialogue_text_id = dialogue_text.get_id();

    // Pause the game while dialogue is showing
    Engine::Time::set_time_scale(0.0001f); // Can't set to 0 or input won't work

    Log::info("Old man dialogue spawned!");
  }

  void destroy_dialogue() {
    if (!dialogue_shown) return;

    if (dialogue_bg_id != 0 && ECS::is_entity_active(ECS::Entity(dialogue_bg_id))) {
      ECS::destroy_entity(ECS::Entity(dialogue_bg_id));
    }
    if (npc_portrait_id != 0 && ECS::is_entity_active(ECS::Entity(npc_portrait_id))) {
      ECS::destroy_entity(ECS::Entity(npc_portrait_id));
    }
    if (dialogue_text_id != 0 && ECS::is_entity_active(ECS::Entity(dialogue_text_id))) {
      ECS::destroy_entity(ECS::Entity(dialogue_text_id));
    }

    dialogue_bg_id = 0;
    npc_portrait_id = 0;
    dialogue_text_id = 0;
    dialogue_shown = false;

    // Resume the game
    Engine::Time::set_time_scale(1.0f);

    // Any valid dialogue close should request gameplay tips overlay
    trigger_tips_overlay_from_gameplay();

    Log::info("Old man dialogue destroyed!");
  }

  void old_man_on_enter(ECS::Entity oldman_entity, ECS::Entity player_entity) {
    Log::info("Player {} entered interaction range", player_entity.get_id());
    spawn_press_e_prompt(oldman_entity);
  }

  bool consume_tips_overlay_request() {
    bool requested = tips_overlay_request_pending;
    tips_overlay_request_pending = false;
    return requested;
  }

  void trigger_tips_overlay_from_gameplay() {
    tips_overlay_request_pending = true;
    auto on_exhausted = Engine::FunctionRegistry::instance().get_function<void()>("gameplay_oldman_dialogue_exhausted");
    if (on_exhausted) {
      Log::info("OldMan: triggering gameplay_oldman_dialogue_exhausted");
      on_exhausted();
    }
    else {
      Log::warning("OldMan: gameplay_oldman_dialogue_exhausted callback not found, trying gameplay_tips_button_click fallback");
      auto on_tips_click = Engine::FunctionRegistry::instance().get_function<void()>("gameplay_tips_button_click");
      if (on_tips_click) {
        on_tips_click();
      }
      else {
        Log::warning("OldMan: gameplay_tips_button_click fallback callback also not found");
      }
    }
  }

  void old_man_on_update(ECS::Entity oldman_entity, ECS::Entity player_entity) {
    // Fade in the press E prompt using unscaled dt (works even when time scale is near-zero)
    if (press_e_prompt_id != 0 && ECS::is_entity_active(ECS::Entity(press_e_prompt_id))) {
      auto* color = ECS::Entity(press_e_prompt_id).get_component<Components::Color>();
      if (color && color->a < 1.0f) {
        float dt = Engine::Time::get_unscaled_fixed_dt();
        color->a += PRESS_E_FADE_SPEED * dt;
        if (color->a > 1.0f) color->a = 1.0f;
      }

      // Keep the prompt anchored above the oldman as it moves
      auto* prompt_tf = ECS::Entity(press_e_prompt_id).get_component<Components::Transform2D>();
      auto* oldman_tf = oldman_entity.get_component<Components::Transform2D>();
      if (prompt_tf && oldman_tf) {
        prompt_tf->position.x = oldman_tf->position.x;
        prompt_tf->position.y = oldman_tf->position.y + PRESS_E_PROMPT_OFFSET_Y;
      }
    }

    // Manual edge detection for E key (since time scale affects is_pressed behavior)
    bool e_key_down = Engine::Input::is_held_raw(Engine::KeyCode::E);

    // Only trigger on the rising edge (key just went from up to down)
    if (e_key_down && !e_key_was_down) {
      if (dialogue_shown) {
        destroy_dialogue();
      }
      else {
        spawn_dialogue(player_entity);
      }
    }

    e_key_was_down = e_key_down;
  }

  void old_man_on_exit(ECS::Entity interactable, ECS::Entity player_entity) {
    Log::info("Player {} exited interaction range of entity {}", player_entity.get_id(), interactable.get_id());
    destroy_press_e_prompt();
    destroy_dialogue();
  }
} // namespace OldManCallbacks


REGISTER_FUNCTION(OldManCallbacks::old_man_on_enter)
REGISTER_FUNCTION(OldManCallbacks::old_man_on_update)
REGISTER_FUNCTION(OldManCallbacks::old_man_on_exit)

namespace {
  struct OldManLegacyAliasRegistration {
    OldManLegacyAliasRegistration() {
      using Sig = void(Engine::ECS::Entity, Engine::ECS::Entity);
      Engine::FunctionRegistry::instance().register_function<Sig>("old_man_on_enter", std::function<Sig>(OldManCallbacks::old_man_on_enter));
      Engine::FunctionRegistry::instance().register_function<Sig>("old_man_on_update", std::function<Sig>(OldManCallbacks::old_man_on_update));
      Engine::FunctionRegistry::instance().register_function<Sig>("old_man_on_exit", std::function<Sig>(OldManCallbacks::old_man_on_exit));
    }
  };

  static OldManLegacyAliasRegistration s_oldman_legacy_alias_registration;
}