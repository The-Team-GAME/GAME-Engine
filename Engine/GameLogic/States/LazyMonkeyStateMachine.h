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

  inline Components::StateMachine create_state_machine()
  {
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

    Components::State lazy_monkey_chasing("Chasing", "lazy_monkey_chasing_enter", "lazy_monkey_chasing_update", "lazy_monkey_chasing_exit", lazy_monkey_chasing_anim);

    // ========================================================================
    // Lazy Monkey Attacking State
    // ========================================================================
    Components::StateAnimation lazy_monkey_attacking_anim;
    lazy_monkey_attacking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Attack_Spritesheet.png"));
    lazy_monkey_attacking_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 4, 4);
    lazy_monkey_attacking_anim.animation = Components::Animation({ 0.2f, 0.1f, 0.1f, 0.5f }); // Custom frame durations
    lazy_monkey_attacking_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State lazy_monkey_attacking("Attacking", "lazy_monkey_attacking_enter", "lazy_monkey_attacking_update", "lazy_monkey_attacking_exit", lazy_monkey_attacking_anim);

    // ========================================================================
    // Lazy Monkey Hit State
    // ========================================================================
    Components::StateAnimation lazy_monkey_hit_anim;
    lazy_monkey_hit_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Death_Spritesheet.png"));
    lazy_monkey_hit_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 3, 3);
    lazy_monkey_hit_anim.animation = Components::Animation({ 0.4f }); // Single frame (index 0) for 1.0s
    lazy_monkey_hit_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State lazy_monkey_hit("Hit", "lazy_monkey_hit_enter", "lazy_monkey_hit_update", "lazy_monkey_hit_exit", lazy_monkey_hit_anim);

    // ========================================================================
    // Lazy Monkey Death State
    // ========================================================================
    Components::StateAnimation lazy_monkey_death_anim;
    lazy_monkey_death_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Death_Spritesheet.png"));
    lazy_monkey_death_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 3, 3);
    lazy_monkey_death_anim.animation = Components::Animation({ 0.2f, 0.1f, 1.5f }); // Full death animation
    lazy_monkey_death_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State lazy_monkey_death("Death", "lazy_monkey_death_enter", "lazy_monkey_death_update", "lazy_monkey_death_exit", lazy_monkey_death_anim);

    // ========================================================================
    // Build and return state machine
    // ========================================================================
    Components::StateMachine lazy_monkey_sm;
    lazy_monkey_sm.add_state(lazy_monkey_idle);
    lazy_monkey_sm.add_state(lazy_monkey_chasing);
    lazy_monkey_sm.add_state(lazy_monkey_attacking);
    lazy_monkey_sm.add_state(lazy_monkey_hit);
    lazy_monkey_sm.add_state(lazy_monkey_death);
    lazy_monkey_sm.set_initial_state("Idle");

    return lazy_monkey_sm;
  }
}
