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

namespace GamerMonkeyStates
{
  // Gamer Monkey dimensions
  constexpr f32 WIDTH = 400.0f;
  constexpr f32 HEIGHT = 400.0f;
  constexpr f32 ATTACK_WIDTH = 600.0f;
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
    Gamer_monkey_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/idle.png"));
    Gamer_monkey_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 5, 5);
    Gamer_monkey_idle_anim.animation = Components::Animation(4, 0.15f);
    Gamer_monkey_idle_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_idle("Idle", "Gamer_monkey_idle_enter", "Gamer_monkey_idle_update", "", Gamer_monkey_idle_anim);

    // ========================================================================
    // Gamer Monkey Dash State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_dashing_anim;
    Gamer_monkey_dashing_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/dash.png"));
    Gamer_monkey_dashing_anim.texture_atlas = Components::TextureAtlas(64, 64, 2, 6, 12);
    Gamer_monkey_dashing_anim.animation = Components::Animation(12, 0.1f);
    Gamer_monkey_dashing_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_dashing("Dash", "Gamer_monkey_dashing_enter", "Gamer_monkey_dashing_update", "Gamer_monkey_dashing_exit", Gamer_monkey_dashing_anim, footstep_audio);

    // ========================================================================
    // Gamer Monkey Chase State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_chasing_anim;
    Gamer_monkey_chasing_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/dash.png"));
    Gamer_monkey_chasing_anim.texture_atlas = Components::TextureAtlas(64, 64, 2, 6, 12);
    Gamer_monkey_chasing_anim.animation = Components::Animation(12, 0.1f);
    Gamer_monkey_chasing_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_chasing("Chasing", "Gamer_monkey_chasing_enter", "Gamer_monkey_chasing_update", "Gamer_monkey_chasing_exit", Gamer_monkey_chasing_anim, footstep_audio);

    // ========================================================================
    // Gamer Monkey Windup State (uses idle animation)
    // ========================================================================
    Components::StateAnimation Gamer_monkey_windup_anim;
    Gamer_monkey_windup_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/attacking.png"));
    Gamer_monkey_windup_anim.texture_atlas = Components::TextureAtlas(64, 64, 3, 6, 2);
    Gamer_monkey_windup_anim.animation = Components::Animation(2, 0.5f);
    Gamer_monkey_windup_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_windup("Windup", "Gamer_monkey_windup_enter", "Gamer_monkey_windup_update", "", Gamer_monkey_windup_anim);

    // ========================================================================
    // Gamer Monkey Attacking State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_attacking_anim;
    Gamer_monkey_attacking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/attacking.png"));
    Gamer_monkey_attacking_anim.texture_atlas = Components::TextureAtlas(64, 64, 3, 6, 13);
    Gamer_monkey_attacking_anim.animation = Components::Animation({ 0.0f, 0.0f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.5f });
    Gamer_monkey_attacking_anim.scale = Math::Vec2D(ATTACK_WIDTH, ATTACK_HEIGHT);

    Components::State Gamer_monkey_attacking("Attacking", "Gamer_monkey_attacking_enter", "Gamer_monkey_attacking_update", "Gamer_monkey_attacking_exit", Gamer_monkey_attacking_anim, attack_audio);

    // ========================================================================
    // Gamer Monkey Hit State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_hit_anim;
    Gamer_monkey_hit_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/idle.png"));
    Gamer_monkey_hit_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 5, 5);
    Gamer_monkey_hit_anim.animation = Components::Animation(4, 0.15f);  // Changed from Gamer_monkey_idle_anim
    //Gamer_monkey_hit_anim.animation = Components::Animation(std::initializer_list{ 2 }, { 0.4f });
    Gamer_monkey_hit_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_hit("Hit", "Gamer_monkey_hit_enter", "Gamer_monkey_hit_update", "Gamer_monkey_hit_exit", Gamer_monkey_hit_anim, hurt_audio);

    // ========================================================================
    // Gamer Monkey Death State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_death_anim;
    Gamer_monkey_death_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/death.png"));
    Gamer_monkey_death_anim.texture_atlas = Components::TextureAtlas(64, 64, 2, 10, 20);
    Gamer_monkey_death_anim.animation = Components::Animation(20, 0.1f);
    Gamer_monkey_death_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_death("Death", "Gamer_monkey_death_enter", "Gamer_monkey_death_update", "Gamer_monkey_death_exit", Gamer_monkey_death_anim);

    // ========================================================================
    // Build and return state machine
    // ========================================================================
    Components::StateMachine Gamer_monkey_sm;
    Gamer_monkey_sm.add_state(Gamer_monkey_idle);
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
