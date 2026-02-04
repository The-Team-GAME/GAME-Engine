
#include "../../Engine/MonkeyEngine.h"
#include "../../Engine/Systems/GraphicsSystem.h"

namespace OldManCallbacks
{
  // Static entity IDs for dialogue UI (to track and clean up)
  inline Engine::ECS::EntityID dialogue_bg_id = 0;
  inline Engine::ECS::EntityID npc_portrait_id = 0;
  inline Engine::ECS::EntityID dialogue_text_id = 0;
  inline bool dialogue_shown = false;
  inline bool e_key_was_down = false;  // Track previous E key state for edge detection

  void spawn_dialogue(Engine::ECS::Entity player_entity) {
    if (dialogue_shown) return;
    dialogue_shown = true;

    // Get camera position to center dialogue on screen
    auto& camera = Engine::Systems::CameraSystem::get_camera();
    float cam_x = camera.position.x + 800.0f;  // Screen center X
    float cam_y = camera.position.y + 450.0f;  // Screen center Y

    // Create dialogue background - covers whole screen
    auto dialogue_bg = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(cam_x, cam_y, 0.0f, 1600.0f, 900.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/NotBroDialogue.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_to_layer("main_menu_4");
    dialogue_bg_id = dialogue_bg.get_id();

    // Create NPC portrait (NPCOldman.png) - bottom right of screen
    auto npc_portrait = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(cam_x + 509.0f, cam_y + 173.0f, 0.0f, 320.0f, 320.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/NPCOldman.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_to_layer("main_menu_5");
    npc_portrait_id = npc_portrait.get_id();

    // Create dialogue text - bottom of screen
    auto dialogue_text = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(cam_x - 177.0f, cam_y + 270.0f, 0.0f, 631.0f, 166.0f)
      .add_component<Components::Font>(Engine::Resources::load_font("fonts/FiveFontsatFreddy's-Regular.ttf", 48))
      .add_component<Components::Text>("Its time to retrieve your homework boy! Reach the end of the station and take down the big monkey FOR YOUR GPA!", 0.5f)
      .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 1.0f)
      .add_to_layer("main_menu_5");
    dialogue_text_id = dialogue_text.get_id();

    // Pause the game while dialogue is showing
    Engine::Time::set_time_scale(0.0001f); // Can't set to 0 or input won't work

    Log::info("Old man dialogue spawned!");
  }

  void destroy_dialogue() {
    if (!dialogue_shown) return;

    if (dialogue_bg_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(dialogue_bg_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(dialogue_bg_id));
    }
    if (npc_portrait_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(npc_portrait_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(npc_portrait_id));
    }
    if (dialogue_text_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(dialogue_text_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(dialogue_text_id));
    }

    dialogue_bg_id = 0;
    npc_portrait_id = 0;
    dialogue_text_id = 0;
    dialogue_shown = false;

    // Resume the game
    Engine::Time::set_time_scale(1.0f);

    Log::info("Old man dialogue destroyed!");
  }

  void old_man_on_enter(Engine::ECS::Entity interactable, Engine::ECS::Entity player_entity) {
    Log::info("Player {} entered interaction range of entity {}", player_entity.get_id(), interactable.get_id());
  }

  void old_man_on_update(Engine::ECS::Entity interactable, Engine::ECS::Entity player_entity) {
    // Manual edge detection for E key (since time scale affects is_pressed behavior)
    bool e_key_down = Engine::Input::is_held(Engine::KeyCode::E);

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

  void old_man_on_exit(Engine::ECS::Entity interactable, Engine::ECS::Entity player_entity) {
    Log::info("Player {} exited interaction range of entity {}", player_entity.get_id(), interactable.get_id());
    destroy_dialogue();
  }
} // namespace OldManCallbacks


REGISTER_FUNCTION(old_man_on_enter, void(Engine::ECS::Entity, Engine::ECS::Entity), OldManCallbacks::old_man_on_enter);
REGISTER_FUNCTION(old_man_on_update, void(Engine::ECS::Entity, Engine::ECS::Entity), OldManCallbacks::old_man_on_update);
REGISTER_FUNCTION(old_man_on_exit, void(Engine::ECS::Entity, Engine::ECS::Entity), OldManCallbacks::old_man_on_exit);