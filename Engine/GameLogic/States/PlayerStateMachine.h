//-----------------------------------------------------------------------------------
// file:    PlayerStateMachine.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (80%)
//          Primary author
//          Cheng Ming Xian (20%)
//          Co-Author
//
// email:   chew.s@digipen.edu
//          c.mingxian@digipen.edu
//
// brief:   This file defines the player state machine and all player states
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"
#include "../Components/GameplayComponent.h"
#include "PlayerStateCallbacks.h"

namespace PlayerStates
{
  // Player dimensions
  constexpr f32 IDLE_WIDTH = 64.0f;
  constexpr f32 IDLE_HEIGHT = 64.0f;
  constexpr f32 ATTACK_WIDTH = 96.0f;
  constexpr f32 ATTACK_HEIGHT = 64.0f;

  inline Components::StateMachine create_state_machine()
  {
    // ========================================================================
    // Audio Setup
    // ========================================================================
    Components::StateAudio walk_audio;
    walk_audio.sound = Engine::Resources::load_sound("sounds/SFX/Player/Player Footstep.wav");
    walk_audio.loop = true;
    walk_audio.volume = 0.5f;
    walk_audio.stop_on_exit = true;

    Components::StateAudio jump_audio;
    jump_audio.sound = Engine::Resources::load_sound("sounds/SFX/Player/Player Jump (DJ SY Edition).wav");
    jump_audio.loop = false;
    jump_audio.volume = 0.5f;
    jump_audio.stop_on_exit = false;

    Components::StateAudio attack_audio;
    attack_audio.sound = Engine::Resources::load_sound("sounds/SFX/Player/Player Swing.wav");
    attack_audio.loop = false;
    attack_audio.volume = 0.2f;
    attack_audio.stop_on_exit = false;


    // ========================================================================
    // FMOD Audio Setup (NEW SYSTEM - preferred if FMOD banks are available)
    // ========================================================================
    // NOTE: Uncomment these when you have FMOD Studio events created and banks built


    Components::FMODStateAudio fmod_hurt_audio;
    fmod_hurt_audio.event_path = "event:/SFX/PlayerHurt";
    fmod_hurt_audio.loop = false;
    fmod_hurt_audio.volume = 0.6f;
    fmod_hurt_audio.stop_on_state_exit = false;

    Components::FMODStateAudio fmod_walking_audio;
    fmod_walking_audio.event_path = "event:/SFX/PlayerWalking";
    fmod_walking_audio.loop = false;
    fmod_walking_audio.volume = 0.6f;
    fmod_walking_audio.stop_on_state_exit = false;

    Components::FMODStateAudio fmod_jump_audio;
    fmod_jump_audio.event_path = "event:/SFX/PlayerJump";
    fmod_jump_audio.loop = false;
    fmod_jump_audio.volume = 0.6f;
    fmod_jump_audio.stop_on_state_exit = false;

    Components::FMODStateAudio fmod_dash_audio;
    fmod_dash_audio.event_path = "event:/SFX/PlayerDash";
    fmod_dash_audio.loop = false;
    fmod_dash_audio.volume = 0.6f;
    fmod_dash_audio.stop_on_state_exit = false;

    Components::FMODStateAudio fmod_attack_audio;
    fmod_attack_audio.event_path = "event:/SFX/PlayerPunch";
    fmod_attack_audio.loop = false;
    fmod_attack_audio.volume = 0.2f;
    fmod_attack_audio.stop_on_state_exit = false;

    // ========================================================================
    // Player Idle State
    // ========================================================================
    Components::StateAnimation player_idle_anim;
    player_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_idle_spritesheet.png"));
    player_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 24, 24);
    player_idle_anim.animation = Components::Animation(24, 0.1f);
    player_idle_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_idle("Idle", player_idle_anim);

    // ========================================================================
    // Player Walking State
    // ========================================================================
    Components::StateAnimation player_walking_anim;
    player_walking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_run_spritesheet.png"));
    player_walking_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 8, 8);
    player_walking_anim.animation = Components::Animation(8, 0.1f);
    player_walking_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_walking("Walking", player_walking_anim, walk_audio, fmod_walking_audio);

    // ========================================================================
    // Player Jumping State
    // ========================================================================
    Components::StateAnimation player_jumping_anim;
    player_jumping_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_jump_spritesheet.png"));
    player_jumping_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 13, 13);
    player_jumping_anim.animation = Components::Animation(8, 0.1f);
    player_jumping_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_jumping("Jumping", player_jumping_anim, jump_audio, fmod_jump_audio);
    
    // ========================================================================
    // Player Falling State
    // ========================================================================
    Components::StateAnimation player_jumping_anim;
    player_jumping_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_jump_spritesheet.png"));
    player_jumping_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 13, 13);
    player_jumping_anim.animation = Components::Animation(8, 0.1f);
    player_jumping_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_jumping("Falling", player_jumping_anim);

    // ========================================================================
    // Player Dash State
    // ========================================================================
    Components::StateAnimation player_dashinging_anim;
    player_dashinging_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_dash_spritesheet.png"));
    player_dashinging_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 9, 9);
    player_dashinging_anim.animation = Components::Animation(8, 0.1f);
    player_dashinging_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_dash("Dash", player_dashinging_anim, walk_audio, fmod_dash_audio);

    // ========================================================================
    // Player Attack 1 State (First Punch)
    // ========================================================================
    Components::StateAnimation player_attack1_anim;
    player_attack1_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_onehit_spritesheet.png"));
    player_attack1_anim.texture_atlas = Components::TextureAtlas(96, 64, 1, 9, 9);
    player_attack1_anim.animation = Components::Animation({ 0, 1, 2, 3, 4, 5, 6, 7, 8 }, { 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f }, false);
    player_attack1_anim.scale = Math::Vec2D(ATTACK_WIDTH * 2.0f, ATTACK_HEIGHT * 2.0f);

    Components::State player_attack1("Attack1", player_attack1_anim, attack_audio, fmod_attack_audio);

    // ========================================================================
    // Player Attack 2 State (Second Punch)
    // ========================================================================
    Components::StateAnimation player_attack2_anim;
    player_attack2_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_twohit_spritesheet.png"));
    player_attack2_anim.texture_atlas = Components::TextureAtlas(96, 64, 1, 13, 13);
    player_attack2_anim.animation = Components::Animation({ 6, 7, 8, 9, 10, 11, 12 }, { 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f }, false);
    player_attack2_anim.scale = Math::Vec2D(ATTACK_WIDTH * 2.0f, ATTACK_HEIGHT * 2.0f);

    Components::State player_attack2("Attack2", player_attack2_anim, attack_audio, fmod_attack_audio);

    // ========================================================================
    // Player Attack 3 State (Third Punch - Finisher)
    // ========================================================================
    Components::StateAnimation player_attack3_anim;
    player_attack3_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_threehit_spritesheet.png"));
    player_attack3_anim.texture_atlas = Components::TextureAtlas(96, 64, 1, 18, 18);
    player_attack3_anim.animation = Components::Animation({ 9, 10, 11, 12, 13, 14, 15, 16, 17 }, { 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f }, false);
    player_attack3_anim.scale = Math::Vec2D(ATTACK_WIDTH * 2.0f, ATTACK_HEIGHT * 2.0f);

    Components::State player_attack3("Attack3", player_attack3_anim, attack_audio, fmod_attack_audio);

    // ========================================================================
    // Player Death State
    // ========================================================================
    Components::StateAnimation player_death_anim;
    player_death_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_death_spritesheet.png"));
    player_death_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 12, 12);
    player_death_anim.animation = Components::Animation(8, 0.1f);
    player_death_anim.scale = Math::Vec2D(IDLE_WIDTH * 4.0f, IDLE_HEIGHT * 2.0f);

    // TODO Change Death Audio
    Components::State player_dead("Death", player_death_anim, walk_audio);

    // ========================================================================
    // Build and return state machine
    // ========================================================================
    Components::StateMachine player_sm;
    player_sm.blueprint_id = "Player";
    player_sm.add_state(player_idle);
    player_sm.add_state(player_walking);
    player_sm.add_state(player_jumping);
    player_sm.add_state(player_dash);
    player_sm.add_state(player_attack1);
    player_sm.add_state(player_attack2);
    player_sm.add_state(player_attack3);
    player_sm.add_state(player_dead);
    player_sm.set_initial_state("Idle");

    return player_sm;
  }
}
