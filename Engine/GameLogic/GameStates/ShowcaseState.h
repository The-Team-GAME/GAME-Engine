//-----------------------------------------------------------------------------------
// file:    ShowcaseState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the showcase game state that demonstrates core engine
//          features including sprite animation, sound playback, collision events,
//          resource management, and ECS systems working together.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../Components/Components.h"

/* ===================== //
// Custom Events         //
// ===================== */
/*=================================================================================
Struct Brief:
Event dispatched when one entity collects another entity. Includes information
about the collector, the collected item, and points awarded.
==================================================================================*/
struct CollectEvent {
  ECS::EntityID collector;
  ECS::EntityID collected;
  s32 points;
};

/*=================================================================================
Struct Brief:
Event dispatched when one entity attacks another entity. Contains information
about the attacker, target, and damage dealt.
==================================================================================*/
struct AttackEvent {
  ECS::EntityID attacker;
  ECS::EntityID target;
  s32 damage;
};

class ShowcaseState : public Engine::GameState<ShowcaseState> {
private:
  // Entities
  ECS::Entity player;
  ECS::Entity enemy;
  ECS::Entity banana;
  ECS::Entity background_entity;

  // Resources - automatically managed via shared_ptr
  Engine::Graphics::Texture player_idle_texture;
  Engine::Graphics::Texture player_punch_texture;
  Engine::Graphics::Texture player_run_texture;
  Engine::Graphics::Texture banana_texture;
  Engine::Graphics::Texture enemy_texture;
  Engine::Graphics::Texture background_texture;

  Engine::Audio::Sound walk_sound;

  Engine::Graphics::Font title_font;
  Engine::Graphics::Font info_font;

  // Audio channel handles
  Engine::Audio::ChannelHandle walk_sound_channel = Engine::Audio::INVALID_CHANNEL;

  // Player state
  bool is_walking = false;
  bool is_punching = false;
  bool facing_right = true;
  s32 banana_collected = 0;
  f32 punch_timer = 0.0f;
  const f32 punch_duration = 0.6f; // 12 frames * 0.05s

  // Player dimensions
  const f32 player_idle_width = 64.0f;
  const f32 player_idle_height = 64.0f;
  const f32 player_punch_width = 96.0f;
  const f32 player_punch_height = 64.0f;

public:
  void load();
  void init();
  void update();
  void draw();
  void free();
  void unload();
};