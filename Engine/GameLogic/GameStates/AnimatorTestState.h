//-----------------------------------------------------------------------------------
// file:    AnimatorTestState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Test state for demonstrating the generic Animator component that can
//          animate any numeric field in any component using reflection.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Engine/MonkeyEngine.h"

class AnimatorTestState : public Engine::Scene {
public:
  void load() {
    // Load textures
    auto monkey_texture = Engine::Resources::load_texture("images/monkey.png");
    auto banana_texture = Engine::Resources::load_texture("images/banana.png");

    // =========================================================================
    // Example 1: Animate color (fade red channel)
    // =========================================================================
    auto color_entity = Engine::ECS::create_entity();
    color_entity
      .add_component<Components::Transform2D>(200.0f, 300.0f, 0.0f, 100.0f, 100.0f)
      .add_component<Components::Texture>(monkey_texture)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Tag<"AnimatorTestState">>()
      .add_to_layer("gameplay");

    Components::Animator color_anim;
    color_anim.duration = 2.0f;
    color_anim.is_looping = true;
    color_anim.ping_pong = true;

    // Track 1: Animate red channel 1.0 -> 0.0
    Components::AnimationTrack red_track("Components::Color", "r");
    red_track.keyframes.push_back({ 1.0f, 0.0f, "EaseLinearNone" });
    red_track.keyframes.push_back({ 0.0f, 1.0f, "EaseSineInOut" });
    color_anim.tracks.push_back(red_track);

    // Track 2: Animate green channel 1.0 -> 0.5
    Components::AnimationTrack green_track("Components::Color", "g");
    green_track.keyframes.push_back({ 1.0f, 0.0f, "EaseLinearNone" });
    green_track.keyframes.push_back({ 0.5f, 1.0f, "EaseBounceOut" });
    color_anim.tracks.push_back(green_track);

    color_entity.add_component<Components::Animator>(color_anim);

    // =========================================================================
    // Example 2: Animate position using field paths
    // =========================================================================
    auto move_entity = Engine::ECS::create_entity();
    move_entity
      .add_component<Components::Transform2D>(500.0f, 300.0f, 0.0f, 80.0f, 80.0f)
      .add_component<Components::Texture>(banana_texture)
      .add_component<Components::Color>(1.0f, 1.0f, 0.0f, 1.0f)
      .add_component<Components::Tag<"AnimatorTestState">>()
      .add_to_layer("gameplay");

    Components::Animator move_anim;
    move_anim.duration = 3.0f;
    move_anim.is_looping = true;

    // Track: Animate position.x
    Components::AnimationTrack pos_x_track("Components::Transform2D", "position.x");
    pos_x_track.keyframes.push_back({ 500.0f, 0.0f, "EaseLinearNone" });
    pos_x_track.keyframes.push_back({ 700.0f, 0.5f, "EaseBackOut" });
    pos_x_track.keyframes.push_back({ 500.0f, 1.0f, "EaseElasticOut" });
    move_anim.tracks.push_back(pos_x_track);

    // Track: Animate position.y
    Components::AnimationTrack pos_y_track("Components::Transform2D", "position.y");
    pos_y_track.keyframes.push_back({ 300.0f, 0.0f, "EaseLinearNone" });
    pos_y_track.keyframes.push_back({ 200.0f, 0.25f, "EaseQuadOut" });
    pos_y_track.keyframes.push_back({ 400.0f, 0.75f, "EaseBounceOut" });
    pos_y_track.keyframes.push_back({ 300.0f, 1.0f, "EaseSineInOut" });
    move_anim.tracks.push_back(pos_y_track);

    move_entity.add_component<Components::Animator>(move_anim);

    // =========================================================================
    // Example 3: Animate rotation and scale
    // =========================================================================
    auto spin_entity = Engine::ECS::create_entity();
    spin_entity
      .add_component<Components::Transform2D>(800.0f, 300.0f, 0.0f, 60.0f, 60.0f)
      .add_component<Components::Texture>(monkey_texture)
      .add_component<Components::Color>(0.5f, 1.0f, 0.5f, 1.0f)
      .add_component<Components::Tag<"AnimatorTestState">>()
      .add_to_layer("gameplay");

    Components::Animator spin_anim;
    spin_anim.duration = 2.0f;
    spin_anim.is_looping = true;

    // Track: Animate rotation
    Components::AnimationTrack rotation_track("Components::Transform2D", "rotation");
    rotation_track.keyframes.push_back({ 0.0f, 0.0f, "EaseLinearNone" });
    rotation_track.keyframes.push_back({ 360.0f, 1.0f, "EaseLinearNone" });
    spin_anim.tracks.push_back(rotation_track);

    // Track: Animate scale.x
    Components::AnimationTrack scale_x_track("Components::Transform2D", "scale.x");
    scale_x_track.keyframes.push_back({ 60.0f, 0.0f, "EaseLinearNone" });
    scale_x_track.keyframes.push_back({ 100.0f, 0.5f, "EaseSineOut" });
    scale_x_track.keyframes.push_back({ 60.0f, 1.0f, "EaseSineIn" });
    spin_anim.tracks.push_back(scale_x_track);

    // Track: Animate scale.y (same as scale.x for uniform scaling)
    Components::AnimationTrack scale_y_track("Components::Transform2D", "scale.y");
    scale_y_track.keyframes.push_back({ 60.0f, 0.0f, "EaseLinearNone" });
    scale_y_track.keyframes.push_back({ 100.0f, 0.5f, "EaseSineOut" });
    scale_y_track.keyframes.push_back({ 60.0f, 1.0f, "EaseSineIn" });
    spin_anim.tracks.push_back(scale_y_track);

    spin_entity.add_component<Components::Animator>(spin_anim);

    // =========================================================================
    // Example 4: Animate alpha for fade effect
    // =========================================================================
    auto fade_entity = Engine::ECS::create_entity();
    fade_entity
      .add_component<Components::Transform2D>(1000.0f, 300.0f, 0.0f, 80.0f, 80.0f)
      .add_component<Components::Texture>(banana_texture)
      .add_component<Components::Color>(1.0f, 0.5f, 0.5f, 1.0f)
      .add_component<Components::Tag<"AnimatorTestState">>()
      .add_to_layer("gameplay");

    Components::Animator fade_anim;
    fade_anim.duration = 1.5f;
    fade_anim.is_looping = true;
    fade_anim.ping_pong = true;

    // Track: Animate alpha
    Components::AnimationTrack alpha_track("Components::Color", "a");
    alpha_track.keyframes.push_back({ 1.0f, 0.0f, "EaseLinearNone" });
    alpha_track.keyframes.push_back({ 0.2f, 1.0f, "EaseCubicInOut" });
    fade_anim.tracks.push_back(alpha_track);

    fade_entity.add_component<Components::Animator>(fade_anim);

    Log::info("AnimatorTestState loaded with 4 generic animation examples");
  };

  void init() {};

  void update() {
    // Press R to restart animations
    if (Engine::Input::is_pressed(KeyCode::R)) {
      Engine::ECS::for_each<Components::Animator>([](Engine::ECS::Entity, Components::Animator* anim) {
        anim->elapsed = 0.0f;
        anim->start_time = Engine::Time::get_time();
        anim->is_playing = true;
        anim->is_reversed = false;
        anim->has_initialized = false;
        });
      Log::info("Animator animations restarted!");
    }

    // Press SPACE to toggle pause
    if (Engine::Input::is_pressed(KeyCode::Space)) {
      Engine::ECS::for_each<Components::Animator>([](Components::Animator* anim) {
        anim->is_playing = !anim->is_playing;
        if (anim->is_playing) {
          anim->start_time = Engine::Time::get_time() - anim->elapsed;
        }
        });
      Log::info("Animator animations toggled!");
    }
  };

  void draw() {
    Engine::Graphics::clear_screen({ 0.1f, 0.1f, 0.15f, 1.0f });
  };

  void free() {};

  void unload() {
    Engine::ECS::destroy_entities_with<Components::Tag<"AnimatorTestState">>();
  };
};
