//-----------------------------------------------------------------------------------
// file:    CreditsCutsceneState.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Implementation of the ending/credits cutscene state.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "CreditsCutsceneState.h"

namespace {
  static constexpr const char* NEXT_SCENE      = "MainMenuState";
  static constexpr const char* CREDITS_VIDEO   = "videos/Credit_Scene.mpg";
  static constexpr double      FADE_OUT_DURATION = 4.0; // seconds before end to begin fading
}

void CreditsCutsceneState::load()
{
}

void CreditsCutsceneState::init()
{
  if (!player.open(CREDITS_VIDEO))
  {
    Log::error("[CreditsCutsceneState] Failed to load credits video: {}", CREDITS_VIDEO);
    Log::error("[CreditsCutsceneState] Convert mp4 -> mpg with: ./convert_videos.ps1 (ffmpeg required)");

    Engine::EventManager::send_event<Engine::Events::ToastEvent>(
      "Credits video failed to load. Convert mp4 to mpg (convert_videos.ps1)",
      Engine::Events::ToastType::Warning,
      5.0f
    );

    Engine::SceneManager::load_scene(NEXT_SCENE);
    return;
  }

  player.set_volume(1.0f);
  player.play();
  player.set_looping(false);

  last_video_time_ = -1.0;
  fade_alpha_       = 0.0f;
}

void CreditsCutsceneState::update()
{
  auto finish = [this]() {
    player.stop();
    Engine::SceneManager::load_scene(NEXT_SCENE);
  };

  // Skip with Escape or Space
  if (Engine::Input::is_pressed(Engine::KeyCode::Escape) ||
      Engine::Input::is_pressed(Engine::KeyCode::Space))
  {
    finish();
    return;
  }

  // Robust end detection (mirrors EnteringTrainCutsceneState)
  const double duration = player.get_duration();
  const double t        = player.get_time();

  // Fade-out overlay: ramp alpha from 0 -> 1 over the last FADE_OUT_DURATION seconds
  if (duration > 0.0 && t >= 0.0)
  {
    const double time_remaining = duration - t;
    if (time_remaining < FADE_OUT_DURATION)
    {
      fade_alpha_ = static_cast<float>(1.0 - time_remaining / FADE_OUT_DURATION);
      if (fade_alpha_ < 0.0f) fade_alpha_ = 0.0f;
      if (fade_alpha_ > 1.0f) fade_alpha_ = 1.0f;
    }
  }

  if (duration > 0.0)
  {
    if (t >= duration - 0.05)
    {
      finish();
      return;
    }

    // Detect loop / time wrap
    if (last_video_time_ >= 0.0 && t + 0.10 < last_video_time_)
    {
      finish();
      return;
    }
  }
  last_video_time_ = t;

  if (player.has_ended())
  {
    finish();
    return;
  }
}

void CreditsCutsceneState::draw()
{
  Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });

  // Render the video centered in the virtual resolution
  Math::Vec2D vres = Engine::Graphics::get_virtual_resolution();
  player.render(vres.x, vres.y);

  // Black fade-out overlay
  if (fade_alpha_ > 0.0f)
  {
    Math::Vec2D center{ vres.x * 0.5f, vres.y * 0.5f };
    Engine::Graphics::draw_rect_ui(center, vres, 0.0f, { 0.0f, 0.0f, 0.0f, fade_alpha_ });
  }
}

void CreditsCutsceneState::free()
{
  player.stop();
}

void CreditsCutsceneState::unload()
{
  player.close();
}


