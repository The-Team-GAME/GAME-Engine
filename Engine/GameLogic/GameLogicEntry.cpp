//-----------------------------------------------------------------------------------
// file:    GameLogicEntry.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (85%)
//          Primary author
//          Cheng Ming Xian (15%)
//          Co-Author
// 
//
// email:   chew.s@digipen.edu
//          c.mingxian@digipen.edu
// 
// brief:   DLL entry point for GameLogic hot-reload. Exports RegisterGameLogic()
//          and UnregisterGameLogic() functions called by the Engine's
//          HotReloadManager. RegisterGameLogic sets up all game-specific
//          callbacks, scenes, systems, and collision handlers.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "pch.h"
#include "../Engine/HotReloadManager.h"
#include "../Engine/Random.h"

// Game states
#include "GameStates/EmptyState.h"
#include "GameStates/MainMenuState.h"
#include "GameStates/Rubric2500ParticlesState.h"
#include "GameStates/StressTestState.h"
#include "GameStates/BasicState.h"
#include "GameStates/PhysicsTestState.h"
#include "GameStates/DistortionEffectState.h"
#include "GameStates/LuaState.h"
#include "GameStates/BehaviourState.h"
#include "GameStates/GameplayState.h"
#include "GameStates/TipsBookState.h"
#include "GameStates/OldManDialogueState.h"
#include "GameStates/UpgradePickerState.h"
#include "GameStates/PauseState.h"
#include "GameStates/VictoryState.h"
#include "GameStates/OptionsState.h"
#include "GameStates/VideoPlaybackState.h"
#include "GameStates/OpeningCutsceneState.h"
#include "GameStates/EnteringTrainCutsceneState.h"
#include "GameStates/LevelTwoStartState.h"

// Game systems
#include "Systems/HealthSystem.h"
#include "Systems/EnemyHealthSystem.h"
#include "Systems/CatMonkeySummonHealthBarSystem.h"
#include "Systems/CatMonkeySummonWaveHealthBarsSystem.h"
#include "Systems/HeartrateSystem.h"
#include "Systems/DebugHitboxRenderSystem.h"
#include "Systems/InteractableSystem.h"

// Interactable callbacks
#include "InteractableCallbacks/OldMan.h"

// Level transition trigger callbacks
#include "States/LevelTransitionStateCallbacks.h"

// Upgrade system
#include "../Engine/UpgradeManager.h"

// Components (needed for collision callbacks)
#include "Components/GameplayComponent.h"
#include "Components/CatMonkeyBossComponent.h"
#include "Components/EnemyHealthComponent.h"
#include "Components/HeartrateComponent.h"
#include "Components/HealthComponents.h"
#include "Components/InteractableComponents.h"
#include "../Engine/LevelManager.h"

namespace {
  static constexpr const char* PLAYER_HEARTBEAT_EVENT = "event:/SFX/PlayerHeartBeat";
}

// Defined in States/CatMonkeyStateCallbacks.cpp
namespace CatMonkeyStateCallbacks {
  void register_catmonkey_blueprint_runtime();
}

// =============================================================================
// Knockback Helper Function
// =============================================================================
// Helper function to apply knockback to the enemy (any maybe vise versa -> not tested yet )
void apply_knockback_from_player(ECS::Entity entity)
{
  ECS::Entity player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
  Components::Transform2D* player_xform = player_entity.get_component<Components::Transform2D>();
  Components::Player* player_comp = player_entity.get_component<Components::Player>();
  assert(player_xform && player_comp);
  Components::Transform2D* xform = entity.get_component<Components::Transform2D>();
  Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
  assert(xform && rigid);
  // First we check which direction to apply knockback for the enemy
  // If the player is to the right we want the knockback to be to the left
  player_comp->knockback_value.x = player_xform->position.x > xform->position.x ?
    -player_comp->knockback_value.x : player_comp->knockback_value.x;

  // Doing the flip here so on editor side we dont need to deal with negative values
  player_comp->knockback_value.y = -player_comp->knockback_value.y;

  // Then we zero the forces and velocity of the monkey before they change state
  rigid->velocity.zero();
  rigid->force = player_comp->knockback_value;
}

void apply_knockback_to_player(ECS::Entity player_entity, ECS::Entity attack_entity)
{
  Components::Transform2D* player_xform = player_entity.get_component<Components::Transform2D>();
  Components::Player* player_comp = player_entity.get_component<Components::Player>();
  Components::RigidBody* player_rigid = player_entity.get_component<Components::RigidBody>();
  assert(player_xform && player_comp && player_rigid);
  Vec2D knockback_value = player_comp->knockback_recieved;

  Components::Transform2D* xform = attack_entity.get_component<Components::Transform2D>();
  assert(xform);
  // First we check which direction to apply knockback for the enemy
  // If the player is to the right we want the knockback to be to the right
  knockback_value.x = player_xform->position.x < xform->position.x ?
    -knockback_value.x : knockback_value.x;

  // Doing the flip here so on editor side we dont need to deal with negative values
  knockback_value.y = -knockback_value.y;

  // Then we zero the forces and velocity of the monkey before they change state
  player_rigid->velocity.zero();
  player_rigid->force = knockback_value;
}

// =============================================================================
// Collision callback registrations
// =============================================================================
static void register_collision_callbacks()
{
  // Collision between gameplay and ui layers (example)
  Engine::ECS::LayerRegistry::register_collision_callback("gameplay", "ui",
    [](Engine::ECS::Entity entityA, Engine::ECS::Entity entityB)
    {
      Log::info("Collision detected between Entity {} and Entity {}", entityA.get_id(), entityB.get_id());
    });

  // Player attacks hitting enemies
#pragma region hitting enemies
  Engine::ECS::LayerRegistry::register_collision_callback("damage_enemy", "enemy",
    [](Engine::ECS::Entity attack_entity, Engine::ECS::Entity enemy_entity)
    {
      auto* attack = attack_entity.get_component<Components::Attack>();
      auto* enemy_health = enemy_entity.get_component<Components::EnemyHealth>();
      auto* state_machine = enemy_entity.get_component<Components::StateMachine>();
      auto* transform = enemy_entity.get_component<Components::Transform2D>();

      if (!attack) {
        Log::error("Error: Attack component missing from attack hitbox, on entity {}", attack_entity.get_id());
        return;
      }
      if (enemy_health && state_machine) {
        if (!attack->should_apply_damage(enemy_entity)) { return; }

        int damage = attack->damage;
        enemy_health->take_damage(damage);
        Engine::Systems::add_camera_shake(0.2f);
        Log::info("camera shake requested on enemy hit");
        Log::info("Enemy took {} damage! HP: {}/{}", damage, enemy_health->current_hp, enemy_health->max_hp);

        Engine::AudioSystem::playEvent("event:/SFX/PlayerPunch");
        Log::info("Playing FMOD event: event:/SFX/PlayerPunch");

        if (enemy_health->current_hp <= 0) {
          if (state_machine->get_current_state() != "Death") {
            state_machine->change_state("Death");
            Engine::UpgradeManager::instance().add_points(1);
            Log::info("Enemy killed! +1 upgrade point (total: {})", Engine::UpgradeManager::instance().get_points());
          }
        }
        else if (state_machine->get_current_state() != "Hit") {
          state_machine->change_state("Hit");
        }
        apply_knockback_from_player(enemy_entity);

        // Spawn Impact VFX
        if (attack->impact_vfx_texture.texture_handle) {
          auto* collider = enemy_entity.get_component<Components::BoxCollider>();
          float width = 64.0f;
          float height = 64.0f;
          Vec2D offset = { 0, 0 };

          if (collider) {
            width = collider->width;
            height = collider->height;
            offset = collider->offset;
          }

          // Spawn multiple VFX at random spots within the hitbox
          int vfx_count = 1;
          for (int i = 0; i < vfx_count; ++i) {
            float rand_x = Engine::Random::random_range(-width * 0.4f, width * 0.4f);
            float rand_y = Engine::Random::random_range(-height * 0.4f, height * 0.4f);

            auto vfx = Engine::ECS::create_entity()
              .add_component<Components::Transform2D>(
                transform->position.x + offset.x + rand_x,
                transform->position.y + offset.y + rand_y,
                0.0f,
                128.0f,
                128.0f
              )
              .add_component<Components::Parent>(enemy_entity.get_id())
              .add_component<Components::Texture>(attack->impact_vfx_texture)
              .add_component<Components::TextureAtlas>(64, 64, 1, 4, 4)
              .add_component<Components::Animation>(0, 3, 0.05f, false)
              .add_component<Components::Lifetime>(0.2f)
              .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
              .add_to_layer("effect");
          }
        }
      }
      else {
        Log::error("Error: EnemyHealth or StateMachine component missing!");
      }
    });
#pragma endregion
  // Enemy attacks hitting player
#pragma region player getting hit
  Engine::ECS::LayerRegistry::register_collision_callback("damage_player", "player",
    [](Engine::ECS::Entity attack_entity, Engine::ECS::Entity player_entity)
    {
      Components::Attack* attack = attack_entity.get_component<Components::Attack>();
      Components::Health* health = player_entity.get_component<Components::Health>();
      Components::RigidBody* rigid = player_entity.get_component<Components::RigidBody>();
      Components::Player* player_comp = player_entity.get_component<Components::Player>();

      if (!attack) {
        Log::error("Error: Attack component missing from enemy attack hitbox, on entity {}", attack_entity.get_id());
        return;
      }

      if (health && rigid) {
        if (!attack->should_apply_damage(player_entity) || player_comp->damage_immune || player_comp->god_mode) {
          return;
        }
        Log::info("Player took damage!");
        const int previous_health = health->value;

        // Decrease HP by 1 per hit
        health->damage(1);
        Log::info("Player took damage! HP: {}/{}", health->value, health->max);

        // Trigger red vignette flash
        Engine::Systems::DamageVignette::trigger(0.8f);

        Engine::Systems::Physics2DSystem::add_force(*rigid, attack->knockback);

        Engine::AudioSystem::playEvent("event:/SFX/PlayerHurt");
        Log::info("Playing FMOD event: event:/SFX/PlayerHurt");

        if (previous_health > 2 && health->value > 0 && health->value <= 2)
        {
          Engine::AudioSystem::playEvent(PLAYER_HEARTBEAT_EVENT);
          Log::info("Playing FMOD event: {}", PLAYER_HEARTBEAT_EVENT);
        }

        // Trigger death state immediately when HP reaches 0
        if (health->is_dead())
        {
          Engine::AudioSystem::stopEvent(PLAYER_HEARTBEAT_EVENT, true);
          Log::info("Player is dead!");
          Components::StateMachine* state_machine = player_entity.get_component<Components::StateMachine>();
          if (state_machine && state_machine->get_current_state() != "Death")
          {
            state_machine->change_state("Death");
          }
        }
        else
        {
          Components::StateMachine* sm = player_entity.get_component<Components::StateMachine>();
          assert(sm);
          sm->change_state("Stunned");
        }
        apply_knockback_to_player(player_entity, attack_entity);
      }
      else {
        Log::error("Error: Health or RigidBody component missing from player!");
      }
    });
#pragma endregion
  // Projectiles hitting tilemap (despawn)
  Engine::ECS::LayerRegistry::register_collision_callback("projectiles", "tilemap_foreground_mid",
    [](Engine::ECS::Entity projectile_entity, [[maybe_unused]] Engine::ECS::Entity tile_entity)
    {
      Log::info("Projectile hit tile, destroying");
      projectile_entity.destroy();
    });

  // Projectiles hitting player (despawn)
  Engine::ECS::LayerRegistry::register_collision_callback("projectiles", "player",
    [](Engine::ECS::Entity projectile_entity, [[maybe_unused]] Engine::ECS::Entity player_entity)
    {
      Log::info("Projectile hit player, destroying");
      projectile_entity.destroy();
    });

  // Portal volumes: player collides with a scene_change_trigger to transition levels.
  Engine::ECS::LayerRegistry::enable_layer_collision("scene_change_trigger", "player");
  Engine::ECS::LayerRegistry::register_collision_callback("scene_change_trigger", "player",
    [](Engine::ECS::Entity trigger_entity, Engine::ECS::Entity player_entity)
    {
      Log::info("[Portal] Collision callback fired: trigger={}, player={}", trigger_entity.get_id(), player_entity.get_id());
      LevelTransitionStateCallbacks::portal_entering_train_cutscene(trigger_entity, player_entity);
    });
}

static void unregister_collision_callbacks()
{
  Engine::ECS::LayerRegistry::unregister_collision_callback("gameplay", "ui");
  Engine::ECS::LayerRegistry::unregister_collision_callback("damage_enemy", "enemy");
  Engine::ECS::LayerRegistry::unregister_collision_callback("damage_player", "player");
  Engine::ECS::LayerRegistry::unregister_collision_callback("projectiles", "tilemap_foreground_mid");
  Engine::ECS::LayerRegistry::unregister_collision_callback("projectiles", "player");
  Engine::ECS::LayerRegistry::unregister_collision_callback("scene_change_trigger", "player");
}

// =============================================================================
// Upgrade stat applicator registrations
// =============================================================================
static void register_upgrade_applicators()
{
  auto& mgr = Engine::UpgradeManager::instance();

  // "attack_damage" — modifies basic attack combo damage (attack1/2/3_data.dmg)
  mgr.register_stat_applicator("attack_damage",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) return;
      auto* player_comp = player_entity.get_component<Components::Player>();
      if (!player_comp) return;

      if (type == Engine::FileIO::UpgradeType::Flat) {
        player_comp->attack1_data.dmg += value;
        player_comp->attack2_data.dmg += value;
        player_comp->attack3_data.dmg += value;
      }
      else {
        player_comp->attack1_data.dmg *= value;
        player_comp->attack2_data.dmg *= value;
        player_comp->attack3_data.dmg *= value;
      }
      Log::info("[UpgradeApplicator] attack_damage applied — atk1:{} atk2:{} atk3:{}",
        player_comp->attack1_data.dmg, player_comp->attack2_data.dmg, player_comp->attack3_data.dmg);
    });

  // "uppercut_damage" — modifies uppercut_data.dmg
  mgr.register_stat_applicator("uppercut_damage",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) return;
      auto* player_comp = player_entity.get_component<Components::Player>();
      if (!player_comp) return;

      if (type == Engine::FileIO::UpgradeType::Flat)
        player_comp->uppercut_data.dmg += value;
      else
        player_comp->uppercut_data.dmg *= value;
      Log::info("[UpgradeApplicator] uppercut_damage applied — dmg:{}", player_comp->uppercut_data.dmg);
    });

  // "downsmash_damage" — modifies smash_data.dmg
  mgr.register_stat_applicator("downsmash_damage",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) return;
      auto* player_comp = player_entity.get_component<Components::Player>();
      if (!player_comp) return;

      if (type == Engine::FileIO::UpgradeType::Flat)
        player_comp->smash_data.dmg += value;
      else
        player_comp->smash_data.dmg *= value;
      Log::info("[UpgradeApplicator] downsmash_damage applied — dmg:{}", player_comp->smash_data.dmg);
    });

  // "uppercut_cooldown" — Flat: cooldown = max(0, cooldown - value)
  mgr.register_stat_applicator("uppercut_cooldown",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) return;
      auto* player_comp = player_entity.get_component<Components::Player>();
      if (!player_comp) return;

      if (type == Engine::FileIO::UpgradeType::Flat)
        player_comp->uppercut.cooldown = std::max(0.0f, player_comp->uppercut.cooldown - value);
      else
        player_comp->uppercut.cooldown = std::max(0.0f, player_comp->uppercut.cooldown * value);
      Log::info("[UpgradeApplicator] uppercut_cooldown applied — cd:{}", player_comp->uppercut.cooldown);
    });

  // "downsmash_cooldown" — Flat: cooldown = max(0, cooldown - value)
  mgr.register_stat_applicator("downsmash_cooldown",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) return;
      auto* player_comp = player_entity.get_component<Components::Player>();
      if (!player_comp) return;

      if (type == Engine::FileIO::UpgradeType::Flat)
        player_comp->smash.cooldown = std::max(0.0f, player_comp->smash.cooldown - value);
      else
        player_comp->smash.cooldown = std::max(0.0f, player_comp->smash.cooldown * value);
      Log::info("[UpgradeApplicator] downsmash_cooldown applied — cd:{}", player_comp->smash.cooldown);
    });

  // "dash_cooldown" — Flat: cooldown = max(0, cooldown - value)
  mgr.register_stat_applicator("dash_cooldown",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) return;
      auto* player_comp = player_entity.get_component<Components::Player>();
      if (!player_comp) return;

      if (type == Engine::FileIO::UpgradeType::Flat)
        player_comp->dash.cooldown = std::max(0.0f, player_comp->dash.cooldown - value);
      else
        player_comp->dash.cooldown = std::max(0.0f, player_comp->dash.cooldown * value);
      Log::info("[UpgradeApplicator] dash_cooldown applied — cd:{}", player_comp->dash.cooldown);
    });
}

// =============================================================================
// FunctionRegistry callback registrations
// =============================================================================
static void register_function_callbacks()
{
  auto& reg = Engine::FunctionRegistry::instance();

  // Pause menu callbacks
  reg.register_function<void()>("pause_menu_resume", std::function<void()>([]() {
    Log::info("Resuming game from pause menu");
    Engine::SceneManager::pop_scene();
    }));
  reg.register_function<void()>("pause_menu_restart", std::function<void()>([]() {
    Log::info("Restarting level from pause menu");
    Engine::SceneManager::pop_scene();
    const std::string& level_id = Engine::LevelManager::get_active_level_id();
    if (!level_id.empty()) {
      Engine::LevelManager::reload_level(level_id);
    }
    }));
  reg.register_function<void()>("pause_menu_options", std::function<void()>([]() {
    Log::info("Opening options from pause menu");
    }));
  reg.register_function<void()>("pause_menu_exit", std::function<void()>([]() {
    Log::info("Exiting to main menu from pause menu");
    Engine::AudioSystem::stopEvent(PLAYER_HEARTBEAT_EVENT, true);
    Engine::SceneManager::load_scene("MainMenuState");
    }));

  // Pause menu hover callbacks
  reg.register_function<void(Engine::ECS::Entity)>("pause_hover_resume", std::function<void(Engine::ECS::Entity)>([](Engine::ECS::Entity) {
    if (PauseState::menu_sprite_id == 0) return;
    if (!Engine::ECS::is_entity_active(Engine::ECS::Entity(PauseState::menu_sprite_id))) return;
    PauseState::hovered_button = 1;
    auto* tex = Engine::ECS::Entity(PauseState::menu_sprite_id).get_component<Components::Texture>();
    if (tex) *tex = PauseState::resume_selected_tex;
    }));
  reg.register_function<void(Engine::ECS::Entity)>("pause_hover_restart", std::function<void(Engine::ECS::Entity)>([](Engine::ECS::Entity) {
    if (PauseState::menu_sprite_id == 0) return;
    if (!Engine::ECS::is_entity_active(Engine::ECS::Entity(PauseState::menu_sprite_id))) return;
    PauseState::hovered_button = 2;
    auto* tex = Engine::ECS::Entity(PauseState::menu_sprite_id).get_component<Components::Texture>();
    if (tex) *tex = PauseState::restart_selected_tex;
    }));
  reg.register_function<void(Engine::ECS::Entity)>("pause_hover_options", std::function<void(Engine::ECS::Entity)>([](Engine::ECS::Entity) {
    if (PauseState::menu_sprite_id == 0) return;
    if (!Engine::ECS::is_entity_active(Engine::ECS::Entity(PauseState::menu_sprite_id))) return;
    PauseState::hovered_button = 3;
    auto* tex = Engine::ECS::Entity(PauseState::menu_sprite_id).get_component<Components::Texture>();
    if (tex) *tex = PauseState::options_selected_tex;
    }));
  reg.register_function<void(Engine::ECS::Entity)>("pause_hover_exit", std::function<void(Engine::ECS::Entity)>([](Engine::ECS::Entity) {
    if (PauseState::menu_sprite_id == 0) return;
    if (!Engine::ECS::is_entity_active(Engine::ECS::Entity(PauseState::menu_sprite_id))) return;
    PauseState::hovered_button = 4;
    auto* tex = Engine::ECS::Entity(PauseState::menu_sprite_id).get_component<Components::Texture>();
    if (tex) *tex = PauseState::exit_selected_tex;
    }));
  reg.register_function<void(Engine::ECS::Entity)>("pause_hover_none", std::function<void(Engine::ECS::Entity)>([](Engine::ECS::Entity) {
    }));

  reg.register_function<void(Engine::ECS::Entity)>("pause_menu_button_hover", std::function<void(Engine::ECS::Entity)>([](Engine::ECS::Entity entity) {
    auto* color = entity.get_component<Components::Color>();
    if (color) {
      color->a = 1.0f;
    }
    }));

  // Main menu button callbacks
  reg.register_function<void()>("start_game", std::function<void()>([]() {
    Engine::SceneManager::load_scene("OpeningCutsceneState");
    }));
  reg.register_function<void()>("options_menu", std::function<void()>([]() {
    }));
  reg.register_function<void()>("exit_game", std::function<void()>([]() {
    Engine::Core::stop();
    }));

  // Victory interactable callback
  reg.register_function<void(Engine::ECS::Entity, Engine::ECS::Entity)>("show_victory",
    std::function<void(Engine::ECS::Entity, Engine::ECS::Entity)>([](Engine::ECS::Entity, Engine::ECS::Entity) {
      Log::info("Victory! Player reached the goal.");
      Engine::SceneManager::push_scene("VictoryState");
      }));

  reg.register_function<void(Engine::ECS::Entity)>("pause_menu_button_exit_hover", std::function<void(Engine::ECS::Entity)>([](Engine::ECS::Entity entity) {
    auto* color = entity.get_component<Components::Color>();
    if (color) {
      color->a = 0.0f;
    }
    }));
}

// =============================================================================
// Scene and system registrations
// =============================================================================
static void register_scenes()
{
  Engine::SceneManager::register_scene<EmptyState>("EmptyState");
  Engine::SceneManager::register_scene<MainMenuState>("MainMenuState");
  Engine::SceneManager::register_scene<GameplayState>("GameplayState");
  Engine::SceneManager::register_scene<Rubric2500ParticlesState>("Rubric2500ParticlesState");
  Engine::SceneManager::register_scene<StressTestState>("StressTestState");
  Engine::SceneManager::register_scene<BasicState>("BasicState");
  Engine::SceneManager::register_scene<PhysicsTestState>("PhysicsTestState");
  Engine::SceneManager::register_scene<DistortionEffectState>("DistortionEffectState");
  Engine::SceneManager::register_scene<LuaState>("LuaState");
  Engine::SceneManager::register_scene<BehaviourState>("BehaviourState");
  Engine::SceneManager::register_scene<TipsBookState>("TipsBookState");
  Engine::SceneManager::register_scene<OldManDialogueState>("OldManDialogueState");
  Engine::SceneManager::register_scene<UpgradePickerState>("UpgradePickerState");
  Engine::SceneManager::register_scene<PauseState>("PauseState");
  Engine::SceneManager::register_scene<VictoryState>("VictoryState");
  Engine::SceneManager::register_scene<OptionsState>("OptionsState");
  Engine::SceneManager::register_scene<VideoPlaybackState>("VideoPlaybackState");
  Engine::SceneManager::register_scene<OpeningCutsceneState>("OpeningCutsceneState");
  Engine::SceneManager::register_scene<EnteringTrainCutsceneState>("EnteringTrainCutsceneState");
  Engine::SceneManager::register_scene<LevelTwoStartState>("LevelTwoStartState");
}

static void register_systems()
{
  Engine::ECS::add_system<Engine::Systems::HealthSystem>();
  Engine::ECS::add_system<Engine::Systems::EnemyHealthSystem>();
  Engine::ECS::add_system<Engine::Systems::HeartrateSystem>();
  Engine::ECS::add_system<Engine::Systems::InteractableSystem>();
  Engine::ECS::add_system<Engine::Systems::DebugHitboxRenderSystem>();
  Engine::ECS::add_system<Engine::Systems::EnemyHealthBarRectSystem>();
  Engine::ECS::add_system<Engine::Systems::HeartHUDSpriteSystem>();
  Engine::ECS::add_system<Engine::Systems::CatMonkeySummonHealthBarSystem>();
  Engine::ECS::add_system<Engine::Systems::CatMonkeySummonWaveHealthBarsSystem>();
}

static void unregister_systems()
{
  Engine::ECS::remove_system<Engine::Systems::HealthSystem>();
  Engine::ECS::remove_system<Engine::Systems::EnemyHealthSystem>();
  Engine::ECS::remove_system<Engine::Systems::HeartrateSystem>();
  Engine::ECS::remove_system<Engine::Systems::InteractableSystem>();
  Engine::ECS::remove_system<Engine::Systems::DebugHitboxRenderSystem>();
  Engine::ECS::remove_system<Engine::Systems::EnemyHealthBarRectSystem>();
  Engine::ECS::remove_system<Engine::Systems::HeartHUDSpriteSystem>();
  Engine::ECS::remove_system<Engine::Systems::CatMonkeySummonHealthBarSystem>();
  Engine::ECS::remove_system<Engine::Systems::CatMonkeySummonWaveHealthBarsSystem>();
}

// =============================================================================
// DLL Exports
// =============================================================================
extern "C" __declspec(dllexport) void RegisterGameLogic()
{
  Log::info("[GameLogic] Registering game logic...");

  register_collision_callbacks();
  register_function_callbacks();
  CatMonkeyStateCallbacks::register_catmonkey_blueprint_runtime();
  register_upgrade_applicators();
  register_scenes();
  register_systems();

  Log::info("[GameLogic] Game logic registered successfully");
}

extern "C" __declspec(dllexport) void UnregisterGameLogic()
{
  Log::info("[GameLogic] Unregistering game logic...");

  unregister_collision_callbacks();
  unregister_systems();

  // Clear scenes BEFORE the DLL is unloaded — Scene subclasses
  // (EmptyState, MainMenuState, etc.) live in this DLL, so their
  // shared_ptr destructors must run while vtables are still valid.
  Engine::SceneManager::scenes.clear();
  Engine::SceneManager::scene_order.clear();
  Engine::SceneManager::scene_stack.clear();
  Engine::SceneManager::current_scene = nullptr;

  // Clear StateMachineBlueprintRegistry — its StateBlueprint entries hold
  // std::string members (on_enter / on_update / on_exit) whose heap data
  // must be freed while the DLL is still loaded.  Static constructors in
  // the next DLL load will re-register all blueprints automatically.
  Engine::StateMachineBlueprintRegistry::instance().clear_all_blueprints();

  // Clear UpgradeManager stat applicators — their lambdas capture code
  // in this DLL, so must be destroyed before unload.
  Engine::UpgradeManager::instance().unregister_all_stat_applicators();

  // Clear FunctionRegistry — its std::any values hold std::function
  // objects whose destructors reference code in this DLL.
  // Must be destroyed while the DLL is still loaded.
  Engine::FunctionRegistry::instance().clear_all();

  // Remove DLL-defined component arrays — their ComponentArray<T>
  // vtables live in this DLL and become invalid after unload.
  Engine::ECS::unregister_component<Components::Player>();
  Engine::ECS::unregister_component<Components::LazyMonkey>();
  Engine::ECS::unregister_component<Components::TurretMonkey>();
  Engine::ECS::unregister_component<Components::SlingshotMonkey>();
  Engine::ECS::unregister_component<Components::SprayCanMonkey>();
  Engine::ECS::unregister_component<::Components::CatMonkeyBoss>();
  Engine::ECS::unregister_component<Components::CatMonkeySummonHealthBar>();
  Engine::ECS::unregister_component<Components::CatMonkeySummonWaveHealthBars>();
  Engine::ECS::unregister_component<Components::Attack>();
  Engine::ECS::unregister_component<Components::PendingKnockback>();
  Engine::ECS::unregister_component<Components::EnemyHealth>();
  Engine::ECS::unregister_component<Components::Heartrate>();
  Engine::ECS::unregister_component<Components::Interactable>();

  Log::info("[GameLogic] Game logic unregistered");
}
