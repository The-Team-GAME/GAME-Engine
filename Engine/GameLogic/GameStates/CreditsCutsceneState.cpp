//-----------------------------------------------------------------------------------
// file:    CreditsCutsceneState.cpp
// project: Monkey Stole My GPA
//
// brief:   Implementation of the ending/credits cutscene state.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "CreditsCutsceneState.h"

namespace {
  static constexpr const char* NEXT_SCENE    = "MainMenuState";
  static constexpr const char* CREDITS_VIDEO = "videos/Credit_Scene.mpg";
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
}

void CreditsCutsceneState::free()
{
  player.stop();
}

void CreditsCutsceneState::unload()
{
  player.close();
}
