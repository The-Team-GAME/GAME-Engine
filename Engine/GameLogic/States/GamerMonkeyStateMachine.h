//-----------------------------------------------------------------------------------
// file:    GamerMonkeyStateMachine.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file defines the Gamer monkey state machine and all Gamer monkey states
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../Components/GameplayComponent.h"
#include "GamerMonkeyStateCallbacks.h"

// Register the code-defined state topology (callbacks only)
REGISTER_STATE_MACHINE_BLUEPRINT(GamerMonkey, "Idle",
  { "Idle",       "GamerMonkeyStateCallbacks::idle_on_enter",        "GamerMonkeyStateCallbacks::idle_on_update",        "" },
  { "DashWindup","GamerMonkeyStateCallbacks::dash_windup_on_enter",  "GamerMonkeyStateCallbacks::dash_windup_on_update", "GamerMonkeyStateCallbacks::dash_windup_on_exit" },
  { "Dash",      "GamerMonkeyStateCallbacks::dashing_on_enter",      "GamerMonkeyStateCallbacks::dashing_on_update",     "GamerMonkeyStateCallbacks::dashing_on_exit" },
  { "Chasing",   "GamerMonkeyStateCallbacks::chasing_on_enter",      "GamerMonkeyStateCallbacks::chasing_on_update",     "GamerMonkeyStateCallbacks::chasing_on_exit" },
  { "Windup",    "GamerMonkeyStateCallbacks::windup_on_enter",       "GamerMonkeyStateCallbacks::windup_on_update",      "" },
  { "Attacking", "GamerMonkeyStateCallbacks::attacking_on_enter",    "GamerMonkeyStateCallbacks::attacking_on_update",   "GamerMonkeyStateCallbacks::attacking_on_exit" },
  { "Hit",       "GamerMonkeyStateCallbacks::hit_on_enter",          "GamerMonkeyStateCallbacks::hit_on_update",         "GamerMonkeyStateCallbacks::hit_on_exit" },
  { "Death",     "GamerMonkeyStateCallbacks::death_on_enter",        "GamerMonkeyStateCallbacks::death_on_update",       "GamerMonkeyStateCallbacks::death_on_exit" })

  namespace GamerMonkeyStates
{
  // Gamer Monkey dimensions
  constexpr f32 WIDTH = 400.0f;
  constexpr f32 HEIGHT = 400.0f;
  constexpr f32 ATTACK_WIDTH = 400.0f;
  constexpr f32 ATTACK_HEIGHT = 400.0f;

  inline Components::StateMachine create_state_machine()
  {
    // ========================================================================
    // Audio Setup
    // ========================================================================
    Components::StateAudio footstep_audio;
    footstep_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey SFX/Monkey FootSteps.wav");
    footstep_audio.loop = true;
    footstep_audio.volume = 0.5f;
    footstep_audio.stop_on_exit = true;

    Components::StateAudio attack_audio;
    attack_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey VO/Monkey-2_2_Attack.wav");
    attack_audio.loop = false;
    attack_audio.volume = 0.6f;
    attack_audio.stop_on_exit = false;

    Components::StateAudio hurt_audio;
    hurt_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey VO/Monkey-1_8_Hurt.wav");
    hurt_audio.loop = false;
    hurt_audio.volume = 0.6f;
    hurt_audio.stop_on_exit = false;

    // ========================================================================
    // Gamer Monkey Idle State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_idle_anim;
    Gamer_monkey_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/walk.png"));
    Gamer_monkey_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    Gamer_monkey_idle_anim.animation = Components::Animation(6, 0.2f);
    Gamer_monkey_idle_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_idle("Idle", "GamerMonkeyStateCallbacks::idle_on_enter", "GamerMonkeyStateCallbacks::idle_on_update", "", Gamer_monkey_idle_anim);

    // ========================================================================
    // Gamer Monkey Dash State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_dashing_anim;
    Gamer_monkey_dashing_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/atk.png"));
    // atk.png frames are 96x64; update atlas to match
    Gamer_monkey_dashing_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 25, 25);
    Gamer_monkey_dashing_anim.animation = Components::Animation({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f });
    Gamer_monkey_dashing_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_dashing("Dash", "GamerMonkeyStateCallbacks::dashing_on_enter", "GamerMonkeyStateCallbacks::dashing_on_update", "GamerMonkeyStateCallbacks::dashing_on_exit", Gamer_monkey_dashing_anim, footstep_audio);

    // ========================================================================
    // Gamer Monkey Dash Windup State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_dash_windup_anim;
    Gamer_monkey_dash_windup_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/atk.png"));
    Gamer_monkey_dash_windup_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 25, 14);
    Gamer_monkey_dash_windup_anim.animation = Components::Animation({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.2f, 0.2f, 0.3f, 0.4f, 0.5f });
    Gamer_monkey_dash_windup_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_dash_windup("DashWindup", "GamerMonkeyStateCallbacks::dash_windup_on_enter", "GamerMonkeyStateCallbacks::dash_windup_on_update", "GamerMonkeyStateCallbacks::dash_windup_on_exit", Gamer_monkey_dash_windup_anim);

    // ========================================================================
    // Gamer Monkey Chase State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_chasing_anim;
    Gamer_monkey_chasing_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/walk.png"));
    Gamer_monkey_chasing_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    Gamer_monkey_chasing_anim.animation = Components::Animation(6, 0.2f);
    Gamer_monkey_chasing_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_chasing("Chasing", "GamerMonkeyStateCallbacks::chasing_on_enter", "GamerMonkeyStateCallbacks::chasing_on_update", "GamerMonkeyStateCallbacks::chasing_on_exit", Gamer_monkey_chasing_anim, footstep_audio);

    // ========================================================================
    // Gamer Monkey Windup State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_windup_anim;
    Gamer_monkey_windup_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/atk.png"));
    Gamer_monkey_windup_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 25, 5);
    Gamer_monkey_windup_anim.animation = Components::Animation(5, 0.3f);
    Gamer_monkey_windup_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_windup("Windup", "GamerMonkeyStateCallbacks::windup_on_enter", "GamerMonkeyStateCallbacks::windup_on_update", "", Gamer_monkey_windup_anim);

    // ========================================================================
    // Gamer Monkey Attacking State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_attacking_anim;
    Gamer_monkey_attacking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/atk.png"));
    Gamer_monkey_attacking_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 25, 8);
    Gamer_monkey_attacking_anim.animation = Components::Animation({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.3f, 0.3f });
    Gamer_monkey_attacking_anim.scale = Math::Vec2D(ATTACK_WIDTH, ATTACK_HEIGHT);

    Components::State Gamer_monkey_attacking("Attacking", "GamerMonkeyStateCallbacks::attacking_on_enter", "GamerMonkeyStateCallbacks::attacking_on_update", "GamerMonkeyStateCallbacks::attacking_on_exit", Gamer_monkey_attacking_anim, attack_audio);

    // ========================================================================
    // Gamer Monkey Hit State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_hit_anim;
    Gamer_monkey_hit_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/walk.png"));
    Gamer_monkey_hit_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    Gamer_monkey_hit_anim.animation = Components::Animation(6, 0.2f);  // Changed from Gamer_monkey_idle_anim
    //Gamer_monkey_hit_anim.animation = Components::Animation(std::initializer_list{ 2 }, { 0.4f });
    Gamer_monkey_hit_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_hit("Hit", "GamerMonkeyStateCallbacks::hit_on_enter", "GamerMonkeyStateCallbacks::hit_on_update", "GamerMonkeyStateCallbacks::hit_on_exit", Gamer_monkey_hit_anim, hurt_audio);

    // ========================================================================
    // Gamer Monkey Death State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_death_anim;
    Gamer_monkey_death_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/walk.png"));
    Gamer_monkey_death_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    Gamer_monkey_death_anim.animation = Components::Animation(6, 0.2f);
    Gamer_monkey_death_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_death("Death", "GamerMonkeyStateCallbacks::death_on_enter", "GamerMonkeyStateCallbacks::death_on_update", "GamerMonkeyStateCallbacks::death_on_exit", Gamer_monkey_death_anim);

    // ========================================================================
    // Build and return state machine
    // ========================================================================
    Components::StateMachine Gamer_monkey_sm;
    Gamer_monkey_sm.blueprint_id = "GamerMonkey";
    Gamer_monkey_sm.add_state(Gamer_monkey_idle);
    Gamer_monkey_sm.add_state(Gamer_monkey_dash_windup);
    Gamer_monkey_sm.add_state(Gamer_monkey_dashing);
    Gamer_monkey_sm.add_state(Gamer_monkey_chasing);
    Gamer_monkey_sm.add_state(Gamer_monkey_windup);
    Gamer_monkey_sm.add_state(Gamer_monkey_attacking);
    Gamer_monkey_sm.add_state(Gamer_monkey_hit);
    Gamer_monkey_sm.add_state(Gamer_monkey_death);
    Gamer_monkey_sm.set_initial_state("Idle");

    return Gamer_monkey_sm;
  }
}
