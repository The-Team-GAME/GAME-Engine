//-----------------------------------------------------------------------------------
// file:    PlayerStateMachine.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the player state machine and all player states
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
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
    walk_audio.sound = Engine::Resources::load_sound("sounds/move1.wav");
    walk_audio.loop = true;
    walk_audio.volume = 0.5f;
    walk_audio.stop_on_exit = true;

    Components::StateAudio jump_audio;
    jump_audio.sound = Engine::Resources::load_sound("sounds/jump.wav");
    jump_audio.loop = false;
    jump_audio.volume = 0.5f;
    jump_audio.stop_on_exit = false;

    Components::StateAudio attack_audio;
    attack_audio.sound = Engine::Resources::load_sound("sounds/Punch.wav");
    attack_audio.loop = false;
    attack_audio.volume = 0.5f;
    attack_audio.stop_on_exit = false;

    // ========================================================================
    // Player Idle State
    // ========================================================================
    Components::StateAnimation player_idle_anim;
    player_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_idle_spritesheet.png"));
    player_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 24, 24);
    player_idle_anim.animation = Components::Animation(24, 0.1f);
    player_idle_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_idle("Idle", "player_idle_enter", "player_idle_update", "", player_idle_anim);

    // ========================================================================
    // Player Walking State
    // ========================================================================
    Components::StateAnimation player_walking_anim;
    player_walking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_run_spritesheet.png"));
    player_walking_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 8, 8);
    player_walking_anim.animation = Components::Animation(8, 0.1f);
    player_walking_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_walking("Walking", "player_walking_enter", "player_walking_update", "player_walking_exit", player_walking_anim, walk_audio);

    // ========================================================================
    // Player Jumping State
    // ========================================================================
    Components::StateAnimation player_jumping_anim;
    player_jumping_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_run_spritesheet.png"));
    player_jumping_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 8, 8);
    player_jumping_anim.animation = Components::Animation(8, 0.1f);
    player_jumping_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_jumping("Jumping", "player_jumping_enter", "player_jumping_update", "", player_jumping_anim, jump_audio);

    // ========================================================================
    // Player Attack 1 State (First Punch)
    // ========================================================================
    Components::StateAnimation player_attack1_anim;
    player_attack1_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_onehit_spritesheet.png"));
    player_attack1_anim.texture_atlas = Components::TextureAtlas(96, 64, 1, 9, 9);
    player_attack1_anim.animation = Components::Animation({ 0, 1, 2, 3, 4, 5, 6, 7, 8 }, { 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f }, false);
    player_attack1_anim.scale = Math::Vec2D(ATTACK_WIDTH * 2.0f, ATTACK_HEIGHT * 2.0f);

    Components::State player_attack1("Attack1", "player_attack1_enter", "player_attack1_update", "", player_attack1_anim, attack_audio);

    // ========================================================================
    // Player Attack 2 State (Second Punch)
    // ========================================================================
    Components::StateAnimation player_attack2_anim;
    player_attack2_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_twohit_spritesheet.png"));
    player_attack2_anim.texture_atlas = Components::TextureAtlas(96, 64, 1, 13, 13);
    player_attack2_anim.animation = Components::Animation({ 6, 7, 8, 9, 10, 11, 12 }, { 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f }, false);
    player_attack2_anim.scale = Math::Vec2D(ATTACK_WIDTH * 2.0f, ATTACK_HEIGHT * 2.0f);

    Components::State player_attack2("Attack2", "player_attack2_enter", "player_attack2_update", "", player_attack2_anim, attack_audio);

    // ========================================================================
    // Player Attack 3 State (Third Punch - Finisher)
    // ========================================================================
    Components::StateAnimation player_attack3_anim;
    player_attack3_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_threehit_spritesheet.png"));
    player_attack3_anim.texture_atlas = Components::TextureAtlas(96, 64, 1, 18, 18);
    player_attack3_anim.animation = Components::Animation({ 9, 10, 11, 12, 13, 14, 15, 16, 17 }, { 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f }, false);
    player_attack3_anim.scale = Math::Vec2D(ATTACK_WIDTH * 2.0f, ATTACK_HEIGHT * 2.0f);

    Components::State player_attack3("Attack3", "player_attack3_enter", "player_attack3_update", "", player_attack3_anim, attack_audio);

    // ========================================================================
    // Build and return state machine
    // ========================================================================
    Components::StateMachine player_sm;
    player_sm.add_state(player_idle);
    player_sm.add_state(player_walking);
    player_sm.add_state(player_jumping);
    player_sm.add_state(player_attack1);
    player_sm.add_state(player_attack2);
    player_sm.add_state(player_attack3);
    player_sm.set_initial_state("Idle");

    return player_sm;
  }
}
