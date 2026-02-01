//-----------------------------------------------------------------------------------
// file:    TransformAnimationState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Test state for demonstrating the TransformAnimation component with
//          keyframe-based animations and per-property easing functions.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Engine/MonkeyEngine.h"

class TransformAnimationState : public Engine::Scene {
public:
  void load() {
    // Load textures
    auto monkey_texture = Engine::Resources::load_texture("images/monkey.png");
    auto banana_texture = Engine::Resources::load_texture("images/banana.png");

    // =========================================================================
    // Example 1: Simple position animation with bounce easing
    // =========================================================================
    auto bouncing_entity = Engine::ECS::create_entity();
    bouncing_entity
      .add_component<Components::Transform2D>(200.0f, 200.0f, 0.0f, 64.0f, 64.0f)
      .add_component<Components::Texture>(monkey_texture)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Tag<"TransformAnimationState">>()
      .add_to_layer("gameplay");

    // Create keyframe animation
    Components::TransformAnimation bounce_anim;
    bounce_anim.duration = 2.0f;
    bounce_anim.is_looping = true;
    bounce_anim.ping_pong = true;

    // Keyframe 0: Start position (top)
    bounce_anim.keyframes.push_back({
      {200.0f, 200.0f},  // position
      0.0f,              // rotation
      {64.0f, 64.0f},    // scale
      0.0f,              // time (normalized 0-1)
      "EaseLinearNone",  // position easing
      "EaseLinearNone",  // rotation easing
      "EaseLinearNone"   // scale easing
      });

    // Keyframe 1: End position (bottom) with bounce
    bounce_anim.keyframes.push_back({
      {200.0f, 600.0f},  // position
      0.0f,              // rotation
      {64.0f, 64.0f},    // scale
      1.0f,              // time
      "EaseBounceOut",   // bounce on position
      "EaseLinearNone",
      "EaseLinearNone"
      });

    bouncing_entity.add_component<Components::TransformAnimation>(bounce_anim);

    // =========================================================================
    // Example 2: Complex animation with different easing per property
    // =========================================================================
    auto complex_entity = Engine::ECS::create_entity();
    complex_entity
      .add_component<Components::Transform2D>(600.0f, 400.0f, 0.0f, 48.0f, 48.0f)
      .add_component<Components::Texture>(banana_texture)
      .add_component<Components::Color>(1.0f, 1.0f, 0.0f, 1.0f)
      .add_component<Components::Tag<"TransformAnimationState">>()
      .add_to_layer("gameplay");

    Components::TransformAnimation complex_anim;
    complex_anim.duration = 3.0f;
    complex_anim.is_looping = true;

    // Keyframe 0: Start
    complex_anim.keyframes.push_back({
      {600.0f, 400.0f}, 0.0f, {48.0f, 48.0f}, 0.0f,
      "EaseLinearNone", "EaseLinearNone", "EaseLinearNone"
      });

    // Keyframe 1: Move right + rotate + scale up
    complex_anim.keyframes.push_back({
      {900.0f, 300.0f}, 180.0f, {96.0f, 96.0f}, 0.33f,
      "EaseQuadOut",    // smooth deceleration on position
      "EaseSineInOut",  // smooth rotation
      "EaseElasticOut"  // bouncy scale
      });

    // Keyframe 2: Move down + more rotation
    complex_anim.keyframes.push_back({
      {900.0f, 500.0f}, 360.0f, {48.0f, 48.0f}, 0.66f,
      "EaseBackIn",     // overshoot position
      "EaseLinearNone",
      "EaseBounceOut"   // bounce scale back down
      });

    // Keyframe 3: Return to start
    complex_anim.keyframes.push_back({
      {600.0f, 400.0f}, 720.0f, {48.0f, 48.0f}, 1.0f,
      "EaseCubicInOut",
      "EaseExpoOut",
      "EaseLinearNone"
      });

    complex_entity.add_component<Components::TransformAnimation>(complex_anim);

    // =========================================================================
    // Example 3: Relative animation (adds to current transform)
    // =========================================================================
    auto relative_entity = Engine::ECS::create_entity();
    relative_entity
      .add_component<Components::Transform2D>(1100.0f, 400.0f, 0.0f, 64.0f, 64.0f)
      .add_component<Components::Texture>(monkey_texture)
      .add_component<Components::Color>(0.5f, 1.0f, 0.5f, 1.0f)
      .add_component<Components::Tag<"TransformAnimationState">>()
      .add_to_layer("gameplay");

    Components::TransformAnimation relative_anim;
    relative_anim.duration = 1.5f;
    relative_anim.is_looping = true;
    relative_anim.ping_pong = true;
    relative_anim.use_relative = true;  // Values are ADDED to initial transform

    // Keyframe 0: No offset (at initial position)
    relative_anim.keyframes.push_back({
      {0.0f, 0.0f},     // offset from initial
      0.0f,             // rotation offset
      {0.0f, 0.0f},     // scale offset (0 = no change)
      0.0f,
      "EaseLinearNone", "EaseLinearNone", "EaseLinearNone"
      });

    // Keyframe 1: Move up 100px, rotate 45 degrees, scale +32px (64->96)
    relative_anim.keyframes.push_back({
      {0.0f, -100.0f},  // move up
      45.0f,            // rotate
      {32.0f, 32.0f},   // scale offset (+32 to each axis)
      1.0f,
      "EaseSineInOut",
      "EaseBackOut",
      "EaseElasticOut"
      });

    relative_entity.add_component<Components::TransformAnimation>(relative_anim);

    // =========================================================================
    // Example 4: UI-style pulse animation
    // =========================================================================
    auto pulse_entity = Engine::ECS::create_entity();
    pulse_entity
      .add_component<Components::Transform2D>(400.0f, 600.0f, 0.0f, 80.0f, 80.0f)
      .add_component<Components::Texture>(banana_texture)
      .add_component<Components::Color>(1.0f, 0.5f, 0.5f, 1.0f)
      .add_component<Components::Tag<"TransformAnimationState">>()
      .add_to_layer("gameplay");

    Components::TransformAnimation pulse_anim;
    pulse_anim.duration = 0.8f;
    pulse_anim.is_looping = true;
    pulse_anim.use_relative = true;

    // Pulse: scale from 80 -> 104 (+24) -> 80
    pulse_anim.keyframes.push_back({
      {0.0f, 0.0f}, 0.0f, {0.0f, 0.0f}, 0.0f,
      "EaseLinearNone", "EaseLinearNone", "EaseLinearNone"
      });
    pulse_anim.keyframes.push_back({
      {0.0f, 0.0f}, 0.0f, {24.0f, 24.0f}, 0.5f,
      "EaseLinearNone", "EaseLinearNone", "EaseSineOut"
      });
    pulse_anim.keyframes.push_back({
      {0.0f, 0.0f}, 0.0f, {0.0f, 0.0f}, 1.0f,
      "EaseLinearNone", "EaseLinearNone", "EaseSineIn"
      });

    pulse_entity.add_component<Components::TransformAnimation>(pulse_anim);

    Log::info("TransformAnimationState loaded with 4 animation examples");
  };

  void init() {};

  void update() {
    // Press R to restart animations
    if (Engine::Input::is_pressed(KeyCode::R)) {
      Engine::ECS::for_each<Components::TransformAnimation>([](Engine::ECS::Entity entity, Components::TransformAnimation* anim) {
        anim->elapsed = 0.0f;
        anim->start_time = Engine::Time::get_time();
        anim->is_playing = true;
        anim->is_reversed = false;
        anim->has_initialized = false;
        });
      Log::info("Animations restarted!");
    }

    // Press SPACE to toggle pause
    if (Engine::Input::is_pressed(KeyCode::Space)) {
      Engine::ECS::for_each<Components::TransformAnimation>([](Components::TransformAnimation* anim) {
        anim->is_playing = !anim->is_playing;
        if (anim->is_playing) {
          // Adjust start_time to account for pause
          anim->start_time = Engine::Time::get_time() - anim->elapsed;
        }
        });
      Log::info("Animations toggled!");
    }
  };

  void draw() {
    Engine::Graphics::clear_screen({ 0.15f, 0.15f, 0.2f, 1.0f });
  };

  void free() {};

  void unload() {
    Engine::ECS::destroy_entities_with<Components::Tag<"TransformAnimationState">>();
  };
};
