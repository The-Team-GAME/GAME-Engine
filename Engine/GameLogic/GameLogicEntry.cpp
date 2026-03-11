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
#include "GameStates/PauseState.h"
#include "GameStates/VictoryState.h"
#include "GameStates/OptionsState.h"
#include "GameStates/VideoPlaybackState.h"

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
// Knockback Helper Function
// =============================================================================
// Helper function to apply knockback to the enemy (any maybe vise versa -> not tested yet )
void apply_knockback_from_player(ECS::Entity entity)
{
  // Check which direction the punching bag is being hit from
  ECS::Entity player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
  Components::Transform2D* player_xform = player_entity.get_component<Components::Transform2D>();
  Components::Player* player_comp = player_entity.get_component<Components::Player>();

  Components::Transform2D* xform = entity.get_component<Components::Transform2D>();
  Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();

  // First we check which direction to apply knockback for the enemy
  // If the player is to the right we want the knockback to be to the left
  player_comp->knockback_value.x = player_xform->position.x > xform->position.x ?
    -player_comp->knockback_value.x : player_comp->knockback_value.x;

  // Then we zero the forces and velocity of the monkey before they change state
  rigid->velocity.zero();
  rigid->force = player_comp->knockback_value;
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
  Engine::ECS::LayerRegistry::register_collision_callback("damage_player", "player",
    [](Engine::ECS::Entity attack_entity, Engine::ECS::Entity player_entity)
    {
      Log::info("Player took damage!");

      Components::Attack* attack = attack_entity.get_component<Components::Attack>();
      Components::Health* health = player_entity.get_component<Components::Health>();
      Components::RigidBody* rigid = player_entity.get_component<Components::RigidBody>();
      Components::Player* player_comp = player_entity.get_component<Components::Player>();

      if (!attack) {
        Log::error("Error: Attack component missing from enemy attack hitbox, on entity {}", attack_entity.get_id());
        return;
      }

      if (health && rigid) {
        if (!attack->should_apply_damage(player_entity) || player_comp->damage_immune) {
          return;
        }

        // Decrease HP by 1 per hit
        health->damage(1);
        Log::info("Player took damage! HP: {}/{}", health->value, health->max);

        // Trigger red vignette flash
        Engine::Systems::DamageVignette::trigger(0.8f);

        Engine::Systems::Physics2DSystem::add_force(*rigid, attack->knockback);

        Engine::AudioSystem::playEvent("event:/SFX/PlayerHurt");
        Log::info("Playing FMOD event: event:/SFX/PlayerHurt");

        Log::info("Player knocked back! Force applied: ({}, {})", attack->knockback.x, attack->knockback.y);

        // Trigger death state immediately when HP reaches 0
        if (health->is_dead())
        {
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
      }
      else {
        Log::error("Error: Health or RigidBody component missing from player!");
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

  // "base_attack_damage" — modifies only Attack1/Attack2/Attack3 damage
  auto base_attack_damage_applicator = [](Engine::FileIO::UpgradeType type, float value) {
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    if (!player_entity.is_valid()) {
      Log::warning("[UpgradeApplicator] No player entity found for 'base_attack_damage'");
      return;
    }
    auto* player = player_entity.get_component<Components::Player>();
    if (!player) return;

    auto apply_flat = [](float& dmg, int flat) {
      dmg += static_cast<float>(flat);
      };

    auto apply_multiplier = [](float& dmg, float mult) {
      int scaled = static_cast<int>(std::lround(dmg * mult));
      if (scaled < 1) scaled = 1;
      dmg = static_cast<float>(scaled);
      };

    if (type == Engine::FileIO::UpgradeType::Flat) {
      int flat = static_cast<int>(std::lround(value));
      if (flat <= 0) flat = 1;

      apply_flat(player->attack1_data.dmg, flat);
      apply_flat(player->attack2_data.dmg, flat);
      apply_flat(player->attack3_data.dmg, flat);
    }
    else {
      apply_multiplier(player->attack1_data.dmg, value);
      apply_multiplier(player->attack2_data.dmg, value);
      apply_multiplier(player->attack3_data.dmg, value);
    }
    Log::info("[UpgradeApplicator] base_attack_damage applied — atk1:{} atk2:{} atk3:{}",
      player->attack1_data.dmg, player->attack2_data.dmg, player->attack3_data.dmg);
    };

  mgr.register_stat_applicator("base_attack_damage", base_attack_damage_applicator);
  mgr.register_stat_applicator("damage", base_attack_damage_applicator); // legacy alias

  // "uppercut_damage" — modifies only Uppercut damage
  auto uppercut_damage_applicator = [](Engine::FileIO::UpgradeType type, float value) {
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    if (!player_entity.is_valid()) {
      Log::warning("[UpgradeApplicator] No player entity found for 'uppercut_damage'");
      return;
    }
    auto* player = player_entity.get_component<Components::Player>();
    if (!player) return;

    auto apply_flat = [](float& dmg, int flat) {
      dmg += static_cast<float>(flat);
      };

    auto apply_multiplier = [](float& dmg, float mult) {
      int scaled = static_cast<int>(std::lround(dmg * mult));
      if (scaled < 1) scaled = 1;
      dmg = static_cast<float>(scaled);
      };

    if (type == Engine::FileIO::UpgradeType::Flat) {
      int flat = static_cast<int>(std::lround(value));
      if (flat <= 0) flat = 1;
      apply_flat(player->uppercut_data.dmg, flat);
    }
    else {
      apply_multiplier(player->uppercut_data.dmg, value);
    }

    Log::info("[UpgradeApplicator] uppercut_damage applied — uppercut:{}",
      player->uppercut_data.dmg);
    };

  mgr.register_stat_applicator("uppercut_damage", uppercut_damage_applicator);
  mgr.register_stat_applicator("uppercut damage", uppercut_damage_applicator); // legacy alias

  // "smash_damage" — modifies only Smash damage
  auto smash_damage_applicator = [](Engine::FileIO::UpgradeType type, float value) {
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    if (!player_entity.is_valid()) {
      Log::warning("[UpgradeApplicator] No player entity found for 'smash_damage'");
      return;
    }
    auto* player = player_entity.get_component<Components::Player>();
    if (!player) return;

    auto apply_flat = [](float& dmg, int flat) {
      dmg += static_cast<float>(flat);
      };

    auto apply_multiplier = [](float& dmg, float mult) {
      int scaled = static_cast<int>(std::lround(dmg * mult));
      if (scaled < 1) scaled = 1;
      dmg = static_cast<float>(scaled);
      };

    if (type == Engine::FileIO::UpgradeType::Flat) {
      int flat = static_cast<int>(std::lround(value));
      if (flat <= 0) flat = 1;
      apply_flat(player->smash_data.dmg, flat);
    }
    else {
      apply_multiplier(player->smash_data.dmg, value);
    }

    Log::info("[UpgradeApplicator] smash_damage applied — smash:{}",
      player->smash_data.dmg);
    };

  mgr.register_stat_applicator("smash_damage", smash_damage_applicator);
  mgr.register_stat_applicator("smash damage", smash_damage_applicator); // legacy alias

  // "movement speed" — modifies walk_speed on the Player component
  mgr.register_stat_applicator("movement speed",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) {
        Log::warning("[UpgradeApplicator] No player entity found for 'movement speed'");
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

      constexpr int kMaxHP = 5;
      health->max = std::clamp(health->max, 1, kMaxHP);
      health->value = std::clamp(health->value, 0, health->max);

      if (type == Engine::FileIO::UpgradeType::Flat) {
        // Flat upgrades increase max HP and heal by the same amount, capped at 5 HP.
        int inc = static_cast<int>(std::lround(value));
        if (inc <= 0) inc = 1;

        int old_value = health->value;
        int old_max = health->max;

        int new_max = std::min(kMaxHP, old_max + inc);
        int max_gain = new_max - old_max;
        health->max = new_max;

        // Heal by the same increment only if not already full.
        if (health->value < health->max) {
          health->heal(inc);
        }

        Log::info(
          "[UpgradeApplicator] health flat applied (+{}) — HP: {}/{} -> {}/{} (max_gain {})",
          inc,
          old_value, old_max,
          health->value, health->max,
          max_gain
        );
      }
      else {
        int old_value = health->value;
        int old_max = health->max;

        int new_max = static_cast<int>(std::lround(static_cast<float>(health->max) * value));
        new_max = std::clamp(new_max, 1, kMaxHP);
        int gained = std::max(0, new_max - old_max);
        health->max = new_max;
        if (health->value > health->max) {
          health->value = health->max;
        }
        if (gained > 0 && health->value < health->max) {
          health->heal(gained);
        }

        Log::info(
          "[UpgradeApplicator] health multiplier applied (x{}) — HP: {}/{} -> {}/{} (gained {})",
          value,
          old_value, old_max,
          health->value, health->max,
          gained
        );
      }
    });

  // ===============================================
  // Player action-specific stats (unused atm until implementations**)
  // ===============================================

  // --- Dash ---

  // "dash_cooldown" — reduces / scales the dash cooldown
  //   Flat:       cooldown = max(1, cooldown - value)
  //   Multiplier: cooldown *= value
  mgr.register_stat_applicator("dash_cooldown",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) {
        Log::warning("[UpgradeApplicator] No player entity found for 'dash_cooldown'");
        return;
      }
      auto* player = player_entity.get_component<Components::Player>();
      if (!player) return;

      if (type == Engine::FileIO::UpgradeType::Flat) {
        player->dash.cooldown = std::max(1.0f, player->dash.cooldown - value);
      }
      else {
        player->dash.cooldown *= value;
        if (player->dash.cooldown < 1.0f) player->dash.cooldown = 1.0f;
      }

      player->dash.cd_timer = std::min(player->dash.cd_timer, player->dash.cooldown);
      Log::info("[UpgradeApplicator] dash_cooldown applied — cd:{}", player->dash.cooldown);
    });

  // --- Uppercut ---

  // "uppercut_cooldown" — reduces / scales the uppercut cooldown
  //   Flat:       cooldown = max(1, cooldown - value)
  //   Multiplier: cooldown *= value
  mgr.register_stat_applicator("uppercut_cooldown",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) {
        Log::warning("[UpgradeApplicator] No player entity found for 'uppercut_cooldown'");
        return;
      }
      auto* player = player_entity.get_component<Components::Player>();
      if (!player) return;

      if (type == Engine::FileIO::UpgradeType::Flat) {
        player->uppercut.cooldown = std::max(1.0f, player->uppercut.cooldown - value);
      }
      else {
        player->uppercut.cooldown *= value;
        if (player->uppercut.cooldown < 1.0f) player->uppercut.cooldown = 1.0f;
      }

      Log::info("[UpgradeApplicator] uppercut_cooldown applied — cd:{}", player->uppercut.cooldown);
    });

  // "uppercut_speed" — modifies the vertical launch speed of the uppercut
  //   Flat:       speed += value
  //   Multiplier: speed *= value
  mgr.register_stat_applicator("uppercut_speed",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) {
        Log::warning("[UpgradeApplicator] No player entity found for 'uppercut_speed'");
        return;
      }
      auto* player = player_entity.get_component<Components::Player>();
      if (!player) return;

      if (type == Engine::FileIO::UpgradeType::Flat) {
        player->uppercut.speed += value;
      }
      else {
        player->uppercut.speed *= value;
      }

      Log::info("[UpgradeApplicator] uppercut_speed applied — speed:{}", player->uppercut.speed);
    });

  // --- Smash / Downsmash ---

  // "smash_cooldown" — reduces / scales the smash cooldown
  mgr.register_stat_applicator("smash_cooldown",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) {
        Log::warning("[UpgradeApplicator] No player entity found for 'smash_cooldown'");
        return;
      }
      auto* player = player_entity.get_component<Components::Player>();
      if (!player) return;

      if (type == Engine::FileIO::UpgradeType::Flat) {
        player->smash.cooldown = std::max(1.0f, player->smash.cooldown - value);
      }
      else {
        player->smash.cooldown *= value;
        if (player->smash.cooldown < 1.0f) player->smash.cooldown = 1.0f;
      }

      Log::info("[UpgradeApplicator] smash_cooldown applied — cd:{}", player->smash.cooldown);
    });

  // "smash_speed" — modifies the downward speed of the smash
  mgr.register_stat_applicator("smash_speed",
    [](Engine::FileIO::UpgradeType type, float value) {
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
      if (!player_entity.is_valid()) {
        Log::warning("[UpgradeApplicator] No player entity found for 'smash_speed'");
        return;
      }
      auto* player = player_entity.get_component<Components::Player>();
      if (!player) return;

      if (type == Engine::FileIO::UpgradeType::Flat) {
        player->smash.speed += value;
      }
      else {
        player->smash.speed *= value;
      }

      Log::info("[UpgradeApplicator] smash_speed applied — speed:{}", player->smash.speed);
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
  Engine::SceneManager::register_scene<VideoPlaybackState>("VideoPlaybackState");
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
  Engine::ECS::unregister_component<Components::SlingshotMonkey>();
  Engine::ECS::unregister_component<Components::SprayCanMonkey>();
  Engine::ECS::unregister_component<Components::Attack>();
  Engine::ECS::unregister_component<Components::PendingKnockback>();
  Engine::ECS::unregister_component<Components::EnemyHealth>();
  Engine::ECS::unregister_component<Components::Heartrate>();
  Engine::ECS::unregister_component<Components::Interactable>();

  Log::info("[GameLogic] Game logic unregistered");
}
