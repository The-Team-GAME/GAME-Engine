//-----------------------------------------------------------------------------------
// file:    GameLogicEntry.cpp
// project: Monkey Stole My GPA
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
#include "GameStates/PauseState.h"
#include "GameStates/VictoryState.h"
#include "GameStates/OptionsState.h"

// Game systems
#include "Systems/HealthSystem.h"
#include "Systems/EnemyHealthSystem.h"
#include "Systems/HeartrateSystem.h"
#include "Systems/DebugHitboxRenderSystem.h"
#include "Systems/InteractableSystem.h"

// Interactable callbacks
#include "InteractableCallbacks/OldMan.h"

// Upgrade system
#include "../Engine/UpgradeManager.h"

// Components (needed for collision callbacks)
#include "Components/GameplayComponent.h"
#include "Components/EnemyHealthComponent.h"
#include "Components/HeartrateComponent.h"
#include "Components/HealthComponents.h"
#include "Components/InteractableComponents.h"

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
        if (!attack->should_apply_damage(enemy_entity)) {
          return;
        }

        int damage = attack->damage;
        enemy_health->take_damage(damage);
        Engine::Systems::add_camera_shake(42.0f, 0.16f);
        Log::info("camera shake requested on enemy hit");
        Log::info("Enemy took {} damage! HP: {}/{}", damage, enemy_health->current_hp, enemy_health->max_hp);

        Engine::AudioSystem::playEvent("event:/SFX/PlayerHit");
        Log::info("Playing FMOD event: event:/SFX/PlayerHit");

        if (enemy_health->current_hp <= 0) {
          if (state_machine->get_current_state() != "Death") {
            state_machine->change_state("Death");
            Engine::UpgradeManager::instance().add_points(50);
            Log::info("Enemy killed! +50 upgrade points (total: {})", Engine::UpgradeManager::instance().get_points());
          }
        }
        else if (state_machine->get_current_state() != "Hit") {
          state_machine->change_state("Hit");
        }

        auto* rigid = enemy_entity.get_component<Components::RigidBody>();
        if (rigid) {
          enemy_entity.add_component<Components::Parent>(attack_entity.get_id());
          enemy_entity.add_component<Components::PendingKnockback>(attack->knockback);

          auto* attack_transform = attack_entity.get_component<Components::Transform2D>();
          if (attack_transform) {
            Math::Vec2D dir = attack_transform->position - transform->position;
            transform->position += dir * 0.2f;
          }

          transform->position.y += -10.0f;
          rigid->velocity.zero();
        }

        // Spawn Impact VFX
        if (attack->impact_vfx_texture.texture_handle) {
          auto vfx = Engine::ECS::create_entity()
            .add_component<Components::Transform2D>(
              transform->position.x,
              transform->position.y,
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
      else {
        Log::error("Error: EnemyHealth or StateMachine component missing!");
      }
    });

  // Enemy attacks hitting player
  Engine::ECS::LayerRegistry::register_collision_callback("damage_player", "player",
    [](Engine::ECS::Entity attack_entity, Engine::ECS::Entity player_entity)
    {
      Log::info("Player took damage!");

      auto* attack = attack_entity.get_component<Components::Attack>();
      auto* heartrate = player_entity.get_component<Components::Heartrate>();
      auto* rigid = player_entity.get_component<Components::RigidBody>();

      if (!attack) {
        Log::error("Error: Attack component missing from enemy attack hitbox, on entity {}", attack_entity.get_id());
        return;
      }

      if (heartrate && rigid) {
        if (!attack->should_apply_damage(player_entity)) {
          return;
        }

        int damage = attack->damage;
        heartrate->increase(static_cast<float>(damage));
        Log::info("Player took {} damage! Heartrate: {}", damage, heartrate->heartrate);

        Engine::Systems::Physics2DSystem::add_force(*rigid, attack->knockback);

        Engine::AudioSystem::playEvent("event:/SFX/PlayerHurt");
        Log::info("Playing FMOD event: event:/SFX/PlayerHurt");

        Log::info("Player knocked back! Force applied: ({}, {})", attack->knockback.x, attack->knockback.y);
      }
      else {
        Log::error("Error: Heartrate or RigidBody component missing from player!");
      }
    });

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
}

static void unregister_collision_callbacks()
{
  Engine::ECS::LayerRegistry::unregister_collision_callback("gameplay", "ui");
  Engine::ECS::LayerRegistry::unregister_collision_callback("damage_enemy", "enemy");
  Engine::ECS::LayerRegistry::unregister_collision_callback("damage_player", "player");
  Engine::ECS::LayerRegistry::unregister_collision_callback("projectiles", "tilemap_foreground_mid");
  Engine::ECS::LayerRegistry::unregister_collision_callback("projectiles", "player");
}

// =============================================================================
// Upgrade stat applicator registrations
// =============================================================================
static void register_upgrade_applicators()
{
  auto& mgr = Engine::UpgradeManager::instance();

  // "damage" — modifies all 3 attack damages on the Player component
  mgr.register_stat_applicator("damage",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) {
        Log::warning("[UpgradeApplicator] No player entity found for 'damage'");
        return;
      }
      auto* player = player_entity.get_component<Components::Player>();
      if (!player) return;

      if (type == Engine::FileIO::UpgradeType::Flat) {
        int flat = static_cast<int>(value);
        player->attack1.damage += flat;
        player->attack2.damage += flat;
        player->attack3.damage += flat;
      }
      else {
        player->attack1.damage = static_cast<s32>(static_cast<float>(player->attack1.damage) * value);
        player->attack2.damage = static_cast<s32>(static_cast<float>(player->attack2.damage) * value);
        player->attack3.damage = static_cast<s32>(static_cast<float>(player->attack3.damage) * value);
      }
      Log::info("[UpgradeApplicator] damage applied — atk1:{} atk2:{} atk3:{}",
        player->attack1.damage, player->attack2.damage, player->attack3.damage);
    });

  // "speed" — modifies walk_speed on the Player component
  mgr.register_stat_applicator("speed",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) {
        Log::warning("[UpgradeApplicator] No player entity found for 'speed'");
        return;
      }
      auto* player = player_entity.get_component<Components::Player>();
      if (!player) return;

      if (type == Engine::FileIO::UpgradeType::Flat) {
        player->walk_speed += value;
      }
      else {
        player->walk_speed *= value;
      }
      Log::info("[UpgradeApplicator] speed applied — walk_speed:{}", player->walk_speed);
    });

  // "health" — modifies max HP on the Health component (and heals the added amount for flat)
  mgr.register_stat_applicator("health",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) {
        Log::warning("[UpgradeApplicator] No player entity found for 'health'");
        return;
      }
      auto* health = player_entity.get_component<Components::Health>();
      if (!health) return;

      if (type == Engine::FileIO::UpgradeType::Flat) {
        int flat = static_cast<int>(value);
        health->max += flat;
        health->heal(flat); // also heal the added amount
      }
      else {
        int old_max = health->max;
        health->max = static_cast<int>(static_cast<float>(health->max) * value);
        int gained = health->max - old_max;
        if (gained > 0) health->heal(gained);
      }
      Log::info("[UpgradeApplicator] health applied — HP:{}/{}", health->value, health->max);
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
    Engine::SceneManager::load_scene("GameplayState");
    }));
  reg.register_function<void()>("pause_menu_options", std::function<void()>([]() {
    Log::info("Opening options from pause menu");
    }));
  reg.register_function<void()>("pause_menu_exit", std::function<void()>([]() {
    Log::info("Exiting to main menu from pause menu");
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
    Engine::SceneManager::load_scene("GameplayState");
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
  Engine::SceneManager::register_scene<PauseState>("PauseState");
  Engine::SceneManager::register_scene<VictoryState>("VictoryState");
  Engine::SceneManager::register_scene<OptionsState>("OptionsState");
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
}

// =============================================================================
// DLL Exports
// =============================================================================
extern "C" __declspec(dllexport) void RegisterGameLogic()
{
  Log::info("[GameLogic] Registering game logic...");

  register_collision_callbacks();
  register_function_callbacks();
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
  Engine::ECS::unregister_component<Components::GamerMonkey>();
  Engine::ECS::unregister_component<Components::SprayCanMonkey>();
  Engine::ECS::unregister_component<Components::Attack>();
  Engine::ECS::unregister_component<Components::PendingKnockback>();
  Engine::ECS::unregister_component<Components::EnemyHealth>();
  Engine::ECS::unregister_component<Components::Heartrate>();
  Engine::ECS::unregister_component<Components::Interactable>();

  Log::info("[GameLogic] Game logic unregistered");
}
