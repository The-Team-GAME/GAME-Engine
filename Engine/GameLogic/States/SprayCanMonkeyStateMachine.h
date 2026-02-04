//-----------------------------------------------------------------------------------
// file:    SprayCanMonkeyStateMachine.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the spray can monkey state machine and all spray can monkey states
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../Components/GameplayComponent.h"
#include "SprayCanMonkeyStateCallbacks.h"

namespace SprayCanMonkeyStates
{
  // Spray Can Monkey dimensions
  constexpr f32 WIDTH = 80.0f;
  constexpr f32 HEIGHT = 80.0f;
  constexpr f32 SPRAY_WIDTH = 80.0f;

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

    Components::StateAudio spray_audio;
    spray_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey VO/Monkey-2_2_Attack.wav");
    spray_audio.loop = false;
    spray_audio.volume = 0.6f;
    spray_audio.stop_on_exit = false;

    Components::StateAudio hurt_audio;
    hurt_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey VO/Monkey-1_8_Hurt.wav");
    hurt_audio.loop = false;
    hurt_audio.volume = 0.6f;
    hurt_audio.stop_on_exit = false;

    // ========================================================================
    // Spray Can Monkey Idle State
    // ========================================================================
    Components::StateAnimation spray_monkey_idle_anim;
    spray_monkey_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/SprayPaintMonkeySpriteSheets/Spraypaint_Walk_Spritesheet.png"));
    spray_monkey_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    spray_monkey_idle_anim.animation = Components::Animation(5, 0.15f);
    spray_monkey_idle_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State spray_monkey_idle("Idle", "spraycan_monkey_idle_enter", "spraycan_monkey_idle_update", "", spray_monkey_idle_anim);

    // ========================================================================
    // Spray Can Monkey Chasing State
    // ========================================================================
    Components::StateAnimation spray_monkey_chasing_anim;
    spray_monkey_chasing_anim.texture = Components::Texture(Engine::Resources::load_texture("images/SprayPaintMonkeySpriteSheets/Spraypaint_Walk_Spritesheet.png"));
    spray_monkey_chasing_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    spray_monkey_chasing_anim.animation = Components::Animation(6, 0.1f);
    spray_monkey_chasing_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State spray_monkey_chasing("Chasing", "spraycan_monkey_chasing_enter", "spraycan_monkey_chasing_update", "spraycan_monkey_chasing_exit", spray_monkey_chasing_anim, footstep_audio);

    // ========================================================================
    // Spray Can Monkey Windup State
    // ========================================================================
    Components::StateAnimation spray_monkey_windup_anim;
    spray_monkey_windup_anim.texture = Components::Texture(Engine::Resources::load_texture("images/SprayPaintMonkeySpriteSheets/Spraypaint_Walk_Spritesheet.png"));
    spray_monkey_windup_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    spray_monkey_windup_anim.animation = Components::Animation(4, 0.1f);
    spray_monkey_windup_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State spray_monkey_windup("Windup", "spraycan_monkey_windup_enter", "spraycan_monkey_windup_update", "", spray_monkey_windup_anim);

    // ========================================================================
    // Spray Can Monkey Spraying State
    // ========================================================================
    Components::StateAnimation spray_monkey_spraying_anim;
    spray_monkey_spraying_anim.texture = Components::Texture(Engine::Resources::load_texture("images/SprayPaintMonkeySpriteSheets/Spraypaint_Attack_Spritesheet.png"));
    spray_monkey_spraying_anim.texture_atlas = Components::TextureAtlas(80, 64, 1, 15, 15);
    spray_monkey_spraying_anim.animation = Components::Animation({ 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f });
    spray_monkey_spraying_anim.scale = Math::Vec2D(WIDTH * 3.0f, HEIGHT * 2.0f);

    Components::State spray_monkey_spraying("Spraying", "spraycan_monkey_spraying_enter", "spraycan_monkey_spraying_update", "spraycan_monkey_spraying_exit", spray_monkey_spraying_anim, spray_audio);

    // ========================================================================
    // Spray Can Monkey Hit State
    // ========================================================================
    Components::StateAnimation spray_monkey_hit_anim;
    spray_monkey_hit_anim.texture = Components::Texture(Engine::Resources::load_texture("images/SprayPaintMonkeySpriteSheets/Spraypaint_Walk_Spritesheet.png"));
    spray_monkey_hit_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    spray_monkey_hit_anim.animation = Components::Animation(std::initializer_list{ 2 }, { 0.4f });
    spray_monkey_hit_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State spray_monkey_hit("Hit", "spraycan_monkey_hit_enter", "spraycan_monkey_hit_update", "spraycan_monkey_hit_exit", spray_monkey_hit_anim, hurt_audio);

    // ========================================================================
    // Spray Can Monkey Death State
    // ========================================================================
    Components::StateAnimation spray_monkey_death_anim;
    spray_monkey_death_anim.texture = Components::Texture(Engine::Resources::load_texture("images/SprayPaintMonkeySpriteSheets/Spraypaint_Walk_Spritesheet.png"));
    spray_monkey_death_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    spray_monkey_death_anim.animation = Components::Animation({ 0.2f, 0.1f, 1.5f });
    spray_monkey_death_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State spray_monkey_death("Death", "spraycan_monkey_death_enter", "spraycan_monkey_death_update", "spraycan_monkey_death_exit", spray_monkey_death_anim);

    // ========================================================================
    // Build and return state machine (without reloading state)
    // ========================================================================
    Components::StateMachine spray_monkey_sm;
    spray_monkey_sm.add_state(spray_monkey_idle);
    spray_monkey_sm.add_state(spray_monkey_chasing);
    spray_monkey_sm.add_state(spray_monkey_windup);
    spray_monkey_sm.add_state(spray_monkey_spraying);
    spray_monkey_sm.add_state(spray_monkey_hit);
    spray_monkey_sm.add_state(spray_monkey_death);
    spray_monkey_sm.set_initial_state("Idle");

    return spray_monkey_sm;
  }
}
