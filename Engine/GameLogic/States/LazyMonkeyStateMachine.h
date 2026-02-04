//-----------------------------------------------------------------------------------
// file:    LazyMonkeyStateMachine.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the lazy monkey state machine and all lazy monkey states
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../Components/GameplayComponent.h"
#include "LazyMonkeyStateCallbacks.h"

namespace LazyMonkeyStates
{
  // Lazy Monkey dimensions
  constexpr f32 WIDTH = 64.0f;
  constexpr f32 HEIGHT = 64.0f;
  constexpr f32 ATTACK_WIDTH = 96.0f;
  constexpr f32 ATTACK_HEIGHT = 64.0f;

  inline Components::StateMachine create_state_machine()
  {
    // ========================================================================
    // Audio Setup (OLD SYSTEM - kept as fallback)
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
   /* hurt_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey VO/Monkey-1_8_Hurt.wav");
    hurt_audio.loop = false;
    hurt_audio.volume = 0.6f;
    hurt_audio.stop_on_exit = false;*/

    

    // ========================================================================
    // FMOD Audio Setup (NEW SYSTEM - preferred if FMOD banks are available)
    // ========================================================================
    // NOTE: Uncomment these when you have FMOD Studio events created and banks built
    
    Components::FMODStateAudio fmod_footstep_audio;
    fmod_footstep_audio.event_path = "event:/SFX/MonkeyFootsteps";
    fmod_footstep_audio.loop = true;
    fmod_footstep_audio.volume = 0.5f;
    fmod_footstep_audio.stop_on_state_exit = true;

    Components::FMODStateAudio fmod_attack_audio;
    fmod_attack_audio.event_path = "event:/SFX/MonkeyAttack";
    fmod_attack_audio.loop = false;
    fmod_attack_audio.volume = 0.6f;
    fmod_attack_audio.stop_on_state_exit = false;

    Components::FMODStateAudio fmod_hurt_audio;
    fmod_hurt_audio.event_path = "event:/SFX/MonkeyHurt";
    fmod_hurt_audio.loop = false;
    fmod_hurt_audio.volume = 0.6f;
    fmod_hurt_audio.stop_on_state_exit = false;

    Components::FMODStateAudio fmod_chase_audio;
    fmod_chase_audio.event_path = "event:/SFX/MonkeyChasing";
    fmod_chase_audio.loop = false;
    fmod_chase_audio.volume = 0.6f;
    fmod_chase_audio.stop_on_state_exit = false;

    // ========================================================================
    // Lazy Monkey Idle State
    // ========================================================================
    Components::StateAnimation lazy_monkey_idle_anim;
    lazy_monkey_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Idle_Spritesheet.png"));
    lazy_monkey_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 5, 5);
    lazy_monkey_idle_anim.animation = Components::Animation(5, 0.15f);
    lazy_monkey_idle_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State lazy_monkey_idle("Idle", "lazy_monkey_idle_enter", "lazy_monkey_idle_update", "", lazy_monkey_idle_anim);

    // ========================================================================
    // Lazy Monkey Chasing State
    // ========================================================================
    Components::StateAnimation lazy_monkey_chasing_anim;
    lazy_monkey_chasing_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Walk_Spritesheet.png"));
    lazy_monkey_chasing_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    lazy_monkey_chasing_anim.animation = Components::Animation(6, 0.1f);
    lazy_monkey_chasing_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    // Try FMOD first, fallback to old audio if FMOD fails
    Components::State lazy_monkey_chasing("Chasing", "lazy_monkey_chasing_enter", "lazy_monkey_chasing_update", "lazy_monkey_chasing_exit", lazy_monkey_chasing_anim, footstep_audio, fmod_chase_audio);

    // ========================================================================
    // Lazy Monkey Windup State (uses idle animation)
    // ========================================================================
    Components::StateAnimation lazy_monkey_windup_anim;
    lazy_monkey_windup_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Idle_Spritesheet.png"));
    lazy_monkey_windup_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 5, 5);
    lazy_monkey_windup_anim.animation = Components::Animation(5, 0.15f);
    lazy_monkey_windup_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State lazy_monkey_windup("Windup", "lazy_monkey_windup_enter", "lazy_monkey_windup_update", "", lazy_monkey_windup_anim);

    // ========================================================================
    // Lazy Monkey Attacking State
    // ========================================================================
    Components::StateAnimation lazy_monkey_attacking_anim;
    lazy_monkey_attacking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Attack_Spritesheet.png"));
    lazy_monkey_attacking_anim.texture_atlas = Components::TextureAtlas(96, 64, 1, 5, 5);
    lazy_monkey_attacking_anim.animation = Components::Animation({ 0.2f, 0.1f, 0.1f, 0.1f, 0.5f });
    lazy_monkey_attacking_anim.scale = Math::Vec2D(ATTACK_WIDTH * 2.0f, ATTACK_HEIGHT * 2.0f);

    // Try FMOD first, fallback to old audio if FMOD fails
    Components::State lazy_monkey_attacking("Attacking", "lazy_monkey_attacking_enter", "lazy_monkey_attacking_update", "lazy_monkey_attacking_exit", lazy_monkey_attacking_anim, attack_audio, fmod_attack_audio);

    // ========================================================================
    // Lazy Monkey Hit State
    // ========================================================================
    Components::StateAnimation lazy_monkey_hit_anim;
    lazy_monkey_hit_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Damaged_Spritesheet.png"));
    lazy_monkey_hit_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 3, 3);
    lazy_monkey_hit_anim.animation = Components::Animation(std::initializer_list{ 2 }, { 0.4f });
    lazy_monkey_hit_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    // FMOD only (no fallback) - old audio commented out above
    Components::State lazy_monkey_hit("Hit", "lazy_monkey_hit_enter", "lazy_monkey_hit_update", "lazy_monkey_hit_exit", lazy_monkey_hit_anim, std::nullopt, fmod_hurt_audio);

    // ========================================================================
    // Lazy Monkey Death State
    // ========================================================================
    Components::StateAnimation lazy_monkey_death_anim;
    lazy_monkey_death_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Death_Spritesheet.png"));
    lazy_monkey_death_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 8, 8);
    // lazy_monkey_death_anim.animation = Components::Animation({ 0.2f, 0.1f, 1.5f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f });
    lazy_monkey_death_anim.animation = Components::Animation({ 0.2f, 0.1f, 1.5f });
    lazy_monkey_death_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State lazy_monkey_death("Death", "lazy_monkey_death_enter", "lazy_monkey_death_update", "lazy_monkey_death_exit", lazy_monkey_death_anim);

    // ========================================================================
    // Build and return state machine
    // ========================================================================
    Components::StateMachine lazy_monkey_sm;
    lazy_monkey_sm.add_state(lazy_monkey_idle);
    lazy_monkey_sm.add_state(lazy_monkey_chasing);
    lazy_monkey_sm.add_state(lazy_monkey_windup);
    lazy_monkey_sm.add_state(lazy_monkey_attacking);
    lazy_monkey_sm.add_state(lazy_monkey_hit);
    lazy_monkey_sm.add_state(lazy_monkey_death);
    lazy_monkey_sm.set_initial_state("Idle");

    return lazy_monkey_sm;
  }
}
