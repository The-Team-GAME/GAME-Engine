//-----------------------------------------------------------------------------------
// file:    TurretMonkeyStateMachine.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file defines the Turret monkey state machine and all Turret monkey states
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../Components/GameplayComponent.h"
#include "TurretMonkeyStateCallbacks.h"

namespace TurretMonkeyStates
{
  // Turret Monkey dimensions

  constexpr f32 WIDTH = 135.0f;
  constexpr f32 HEIGHT = 135.0f;
  constexpr f32 TURRET_WIDTH = 200.0f;
  constexpr f32 TURRET_HEIGHT = 135.0f;

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
    // Turret Monkey Idle State
    // ========================================================================
    Components::StateAnimation Turret_monkey_idle_anim;
    Turret_monkey_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/TurretMonkeySpriteSheets/walk.png"));
    Turret_monkey_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 10, 10);
    Turret_monkey_idle_anim.animation = Components::Animation(10, 0.1f);
    Turret_monkey_idle_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Turret_monkey_idle("Idle", "Turret_monkey_idle_enter", "Turret_monkey_idle_update", "", Turret_monkey_idle_anim);

    // ========================================================================
    // Turret Monkey Chase State
    // ========================================================================
    Components::StateAnimation Turret_monkey_chasing_anim;
    Turret_monkey_chasing_anim.texture = Components::Texture(Engine::Resources::load_texture("images/TurretMonkeySpriteSheets/walk.png"));
    Turret_monkey_chasing_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 10, 10);
    Turret_monkey_chasing_anim.animation = Components::Animation(10, 0.1f);
    Turret_monkey_chasing_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Turret_monkey_chasing("Chasing", "Turret_monkey_chasing_enter", "Turret_monkey_chasing_update", "Turret_monkey_chasing_exit", Turret_monkey_chasing_anim, footstep_audio);

    // ========================================================================
    // Turret Monkey Turret State
    // ========================================================================
    Components::StateAnimation Turret_monkey_turret_anim;
    Turret_monkey_turret_anim.texture = Components::Texture(Engine::Resources::load_texture("images/TurretMonkeySpriteSheets/ceiling_atk.png"));
    Turret_monkey_turret_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    Turret_monkey_turret_anim.animation = Components::Animation(6, 0.1f);
    Turret_monkey_turret_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Turret_monkey_turret("Turret", "Turret_monkey_Turret_enter", "Turret_monkey_Turret_update", "Turret_monkey_Turret_exit", Turret_monkey_turret_anim);

    // ========================================================================
    // Turret Monkey Windup State
    // ========================================================================
    Components::StateAnimation Turret_monkey_windup_anim;
    Turret_monkey_windup_anim.texture = Components::Texture(Engine::Resources::load_texture("images/TurretMonkeySpriteSheets/atk.png"));
    Turret_monkey_windup_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 3);
    Turret_monkey_windup_anim.animation = Components::Animation({0.1f,0.3f,0.5f});
    Turret_monkey_windup_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Turret_monkey_windup("Windup", "Turret_monkey_windup_enter", "Turret_monkey_windup_update", "", Turret_monkey_windup_anim);

    // ========================================================================
    // Turret Monkey Attacking State
    // ========================================================================
    Components::StateAnimation Turret_monkey_attacking_anim;
    Turret_monkey_attacking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/TurretMonkeySpriteSheets/atk.png"));
    Turret_monkey_attacking_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    Turret_monkey_attacking_anim.animation = Components::Animation({0.0f,0.0f,0.0f,0.1f,0.1f,0.1f});
    Turret_monkey_attacking_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Turret_monkey_attacking("Attacking", "Turret_monkey_attacking_enter", "Turret_monkey_attacking_update", "Turret_monkey_attacking_exit", Turret_monkey_attacking_anim, attack_audio);

    // ========================================================================
    // Turret Monkey Hit State
    // ========================================================================
    Components::StateAnimation Turret_monkey_hit_anim;
    Turret_monkey_hit_anim.texture = Components::Texture(Engine::Resources::load_texture("images/TurretMonkeySpriteSheets/walk.png"));
    Turret_monkey_hit_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 10, 10);
    Turret_monkey_hit_anim.animation = Components::Animation(10, 0.1f);
    Turret_monkey_hit_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Turret_monkey_hit("Hit", "Turret_monkey_hit_enter", "Turret_monkey_hit_update", "Turret_monkey_hit_exit", Turret_monkey_hit_anim, hurt_audio);

    // ========================================================================
    // Turret Monkey Death State
    // ========================================================================
    Components::StateAnimation Turret_monkey_death_anim;
    Turret_monkey_death_anim.texture = Components::Texture(Engine::Resources::load_texture("images/TurretMonkeySpriteSheets/walk.png"));
    Turret_monkey_death_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 10, 10);
    Turret_monkey_death_anim.animation = Components::Animation(10, 0.1f);
    Turret_monkey_death_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Turret_monkey_death("Death", "Turret_monkey_death_enter", "Turret_monkey_death_update", "Turret_monkey_death_exit", Turret_monkey_death_anim);

    // ========================================================================
    // Build and return state machine
    // ========================================================================
    Components::StateMachine Turret_monkey_sm;
    Turret_monkey_sm.add_state(Turret_monkey_idle);
    Turret_monkey_sm.add_state(Turret_monkey_chasing);
    Turret_monkey_sm.add_state(Turret_monkey_turret);
    Turret_monkey_sm.add_state(Turret_monkey_windup);
    Turret_monkey_sm.add_state(Turret_monkey_attacking);
    Turret_monkey_sm.add_state(Turret_monkey_hit);
    Turret_monkey_sm.add_state(Turret_monkey_death);
    Turret_monkey_sm.set_initial_state("Idle");

    return Turret_monkey_sm;
  }
}
