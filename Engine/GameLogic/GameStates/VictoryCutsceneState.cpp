//-----------------------------------------------------------------------------------
// file:    FinalCutsceneState.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//
// brief:   Plays VictoryCutscene.mpg then transitions to EndCutsceneState.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "VictoryCutsceneState.h"

namespace {
  static constexpr const char* NEXT_SCENE    = "EndCutsceneState";
  static constexpr const char* VICTORY_VIDEO = "videos/VictoryCutscene.mpg";
}

void FinalCutsceneState::load()
{
}

void FinalCutsceneState::init()
{
  last_video_time_ = -1.0;

  if (!player_.open(VICTORY_VIDEO))
  {
    Log::error("[FinalCutsceneState] Failed to load victory video: {}", VICTORY_VIDEO);
    Log::error("[FinalCutsceneState] Convert mp4 -> mpg with: ./convert_videos.ps1 (ffmpeg required)");

    Engine::EventManager::send_event<Engine::Events::ToastEvent>(
      "Victory cutscene failed to load. Convert mp4 to mpg (convert_videos.ps1)",
      Engine::Events::ToastType::Warning,
      5.0f
    );

    Engine::SceneManager::load_scene(NEXT_SCENE);
    return;
  }

  player_.set_volume(1.0f);
  player_.set_looping(false);
  player_.play();
}

void FinalCutsceneState::update()
{
  auto finish = [this]() {
    player_.stop();
    Engine::SceneManager::load_scene(NEXT_SCENE);
  };

  if (Engine::Input::is_pressed(Engine::KeyCode::Escape) ||
      Engine::Input::is_pressed(Engine::KeyCode::Space))
  {
    finish();
    return;
  }

  const double duration = player_.get_duration();
  const double t        = player_.get_time();

  if (duration > 0.0)
  {
    if (t >= duration - 0.05)
    {
      finish();
      return;
    }

    if (last_video_time_ >= 0.0 && t + 0.10 < last_video_time_)
    {
      finish();
      return;
    }
  }
  last_video_time_ = t;

  if (player_.has_ended())
  {
    finish();
    return;
  }
}

void FinalCutsceneState::draw()
{
  Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });

  Math::Vec2D vres = Engine::Graphics::get_virtual_resolution();
  player_.render(vres.x, vres.y);
}

void FinalCutsceneState::free()
{
  player_.stop();
}

void FinalCutsceneState::unload()
{
  player_.close();
}
