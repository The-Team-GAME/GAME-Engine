//-----------------------------------------------------------------------------------
// file:    EnteringTrainCutsceneState.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//
// brief:   Implementation of the LevelTwo "entering train" cutscene.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "EnteringTrainCutsceneState.h"

#include "../../Engine/LevelManager.h"
#include "GameplayState.h"

namespace {
  static constexpr const char* CUTSCENE_MPG = "videos/Cutscene_Entering_Train.mpg";
  //static constexpr const char* CUTSCENE_MP4 = "videos/Cutscene_Entering_Train.mp4";
  static constexpr const char* LEVEL_TWO_PATH = "levels/LevelTwo.json";
}

void EnteringTrainCutsceneState::set_next_level_to_open(const std::string& level_path)
{
  next_level_to_open = level_path;
  open_level_when_done = !next_level_to_open.empty();
}

void EnteringTrainCutsceneState::load()
{
}

void EnteringTrainCutsceneState::init()
{
  // Default next level for this cutscene.
  // (Can be overridden via set_next_level_to_open if needed.)
  if (next_level_to_open.empty()) {
    set_next_level_to_open(LEVEL_TWO_PATH);
  }

  // VideoPlayer is primarily MPEG-1 based; the repo includes a converter script:
  //   ./convert_videos.ps1  (requires ffmpeg)
  // It converts Assets/videos/*.mp4 -> *.mpg for reliable playback.

  bool loaded = false;

  if (player.open(CUTSCENE_MPG)) {
    loaded = true;
  }

  if (!loaded) {
    Log::error("[EnteringTrainCutsceneState] Failed to load cutscene: {}", CUTSCENE_MPG);
    Log::error("[EnteringTrainCutsceneState] If using .mp4, convert to .mpg with: ./convert_videos.ps1 (ffmpeg required)");

    Engine::EventManager::send_event<Engine::Events::ToastEvent>(
      "Cutscene failed to load. Convert mp4 to mpg (convert_videos.ps1)",
      Engine::Events::ToastType::Warning,
      5.0f
    );

    Engine::SceneManager::load_scene("LevelTwoGameplayState");
    return;
  }

  player.set_volume(1.0f);
  player.play();

  // The cutscene should run once, then return to gameplay.
  // Note: VideoPlayer::play() rewinds the decoder; some backends may reset
  // loop flags on rewind, so apply looping AFTER play().
  player.set_looping(false);

  last_video_time_ = -1.0;
}

void EnteringTrainCutsceneState::update()
{
  auto finish_cutscene = [this]() {
    player.stop();
    Engine::SceneManager::load_scene("LevelTwoGameplayState");
  };

  // Allow skipping the cutscene with Escape or Space
  if (Engine::Input::is_pressed(Engine::KeyCode::Escape) ||
      Engine::Input::is_pressed(Engine::KeyCode::Space))
  {
    finish_cutscene();
    return;
  }

  // Robust end condition: if the decoder loops (time wraps), or if time reaches
  // duration, exit even if has_ended() never flips.
  const double duration = player.get_duration();
  const double t = player.get_time();
  if (duration > 0.0)
  {
    // Near end-of-file
    if (t >= duration - 0.05)
    {
      finish_cutscene();
      return;
    }

    // Detect loop/time wrap
    if (last_video_time_ >= 0.0 && t + 0.10 < last_video_time_)
    {
      finish_cutscene();
      return;
    }
  }
  last_video_time_ = t;

  if (player.has_ended())
  {
    finish_cutscene();
    return;
  }
}

void EnteringTrainCutsceneState::draw()
{
  Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });

  Math::Vec2D vres = Engine::Graphics::get_virtual_resolution();
  player.render(vres.x, vres.y);
}

void EnteringTrainCutsceneState::free()
{
  player.stop();
}

void EnteringTrainCutsceneState::unload()
{
  player.close();
}
