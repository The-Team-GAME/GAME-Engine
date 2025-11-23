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
#include "../../Engine/Systems/ParticleSystem.h"
#include "../../Engine/Systems/PhysicsSystem.h"
#include "../../Engine/Systems/ChatBoxSystem.h"

/* ===================== //
// Custom Events         //
// ===================== */
/*=================================================================================
Struct Brief:
Event dispatched when one entity collects another entity. Includes information
about the collector, the collected item, and points awarded.
==================================================================================*/
struct CollectEvent {
  Engine::ECS::EntityID collector;
  Engine::ECS::EntityID collected;
  s32 points;
};

/*=================================================================================
Struct Brief:
Event dispatched when one entity attacks another entity. Contains information
about the attacker, target, and damage dealt.
==================================================================================*/
struct AttackEvent {
  Engine::ECS::EntityID attacker;
  Engine::ECS::EntityID target;
  s32 damage;
};

class ShowcaseState : public Engine::Scene {
private:
  // Entities
  Engine::ECS::Entity camera;
  Engine::ECS::Entity player;
  Engine::ECS::Entity enemy;
  Engine::ECS::Entity banana;
  Engine::ECS::Entity background_entity;
  Engine::ECS::Entity player_hp_bg, player_hp_fill;

  // Text entities
  Engine::ECS::Entity title_text;
  Engine::ECS::Entity info_text;
  Engine::ECS::Entity banana_count_text;
  Engine::ECS::Entity enemy_health_text;
  Engine::ECS::Entity player_health_text;

  Engine::ECS::Entity chatbox_background;
  Engine::ECS::Entity chatbox_text;
  std::unique_ptr<Engine::Systems::ChatBox> chatbox_system;

  Engine::Graphics::Texture background_texture;

  Engine::Audio::Sound walk_sound;
  Engine::Audio::Sound jump_sound;
  Engine::Audio::Sound punch_sound;
  Engine::Audio::Sound bgm_sound;

  Engine::Graphics::Font title_font;
  Engine::Graphics::Font info_font;

  // Audio channel handles
  Engine::Audio::ChannelHandle walk_sound_channel = Engine::Audio::INVALID_CHANNEL;
  Engine::Audio::ChannelHandle bgm_channel = Engine::Audio::INVALID_CHANNEL;

  // Animation state names
  const std::string IDLE_STATE = "idle";
  const std::string RUN_STATE = "run";
  const std::string JUMP_STATE = "jump";
  const std::string PUNCH_STATE = "punch";

  // Player state
  bool facing_right = true;
  s32 banana_collected = 0;

  // Player dimensions
  const f32 player_idle_width = 64.0f;
  const f32 player_idle_height = 64.0f;
  const f32 player_punch_width = 96.0f;
  const f32 player_punch_height = 64.0f;

  // Player health stuff
  f32 hpbar_left = 50.0f, hpbar_top = 320.0f, hpbar_w = 220.0f, hpbar_h = 18.0f;

public:
  void load();
  void init();
  void update();
  void draw();
  void free();
  void unload();
};