//-----------------------------------------------------------------------------------
// file:    EndCutsceneState.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//
// brief:   Plays VictoryCutscene.mpg then EndCutscene.mpg back-to-back before
//          transitioning to CreditsCutsceneState.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "EndCutsceneState.h"

namespace {
  static constexpr const char* NEXT_SCENE     = "CreditsCutsceneState";
  static constexpr const char* VICTORY_VIDEO  = "videos/VictoryCutscene.mpg";
  static constexpr const char* END_VIDEO      = "videos/EndCutscene.mpg";
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void EndCutsceneState::advance_to_end_video()
{
  player_.stop();
  player_.close();

  phase_           = Phase::End;
  last_video_time_ = -1.0;

  if (!player_.open(END_VIDEO))
  {
    Log::error("[EndCutsceneState] Failed to load end video: {}", END_VIDEO);
    Log::error("[EndCutsceneState] Convert mp4 -> mpg with: ./convert_videos.ps1 (ffmpeg required)");

    Engine::EventManager::send_event<Engine::Events::ToastEvent>(
      "End cutscene failed to load. Convert mp4 to mpg (convert_videos.ps1)",
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

// ---------------------------------------------------------------------------
// Scene interface
// ---------------------------------------------------------------------------

void EndCutsceneState::load()
{
}

void EndCutsceneState::init()
{
  phase_           = Phase::Victory;
  last_video_time_ = -1.0;

  if (!player_.open(VICTORY_VIDEO))
  {
    Log::error("[EndCutsceneState] Failed to load victory video: {}", VICTORY_VIDEO);
    Log::error("[EndCutsceneState] Convert mp4 -> mpg with: ./convert_videos.ps1 (ffmpeg required)");

    Engine::EventManager::send_event<Engine::Events::ToastEvent>(
      "Victory cutscene failed to load. Convert mp4 to mpg (convert_videos.ps1)",
      Engine::Events::ToastType::Warning,
      5.0f
    );

    // Skip straight to the end video so as much content as possible plays.
    advance_to_end_video();
    return;
  }

  player_.set_volume(1.0f);
  player_.set_looping(false);
  player_.play();
}

void EndCutsceneState::update()
{
  // Skip both videos and go straight to credits.
  if (Engine::Input::is_pressed(Engine::KeyCode::Escape) ||
      Engine::Input::is_pressed(Engine::KeyCode::Space))
  {
    player_.stop();
    Engine::SceneManager::load_scene(NEXT_SCENE);
    return;
  }

  const double duration = player_.get_duration();
  const double t        = player_.get_time();

  bool video_finished = false;

  if (duration > 0.0)
  {
    if (t >= duration - 0.05)
      video_finished = true;

    // Detect decoder loop / time wrap
    if (!video_finished && last_video_time_ >= 0.0 && t + 0.10 < last_video_time_)
      video_finished = true;
  }

  if (!video_finished)
    video_finished = player_.has_ended();

  last_video_time_ = t;

  if (video_finished)
  {
    if (phase_ == Phase::Victory)
    {
      advance_to_end_video();
    }
    else // Phase::End
    {
      player_.stop();
      Engine::SceneManager::load_scene(NEXT_SCENE);
    }
  }
}

void EndCutsceneState::draw()
{
  Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });

  Math::Vec2D vres = Engine::Graphics::get_virtual_resolution();
  player_.render(vres.x, vres.y);
}

void EndCutsceneState::free()
{
  player_.stop();
}

void EndCutsceneState::unload()
{
  player_.close();
}
