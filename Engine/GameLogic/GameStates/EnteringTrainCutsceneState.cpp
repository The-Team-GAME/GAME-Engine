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

namespace {
  static constexpr const char* CUTSCENE_MPG = "videos/Cutscene_Entering_Train.mpg";
  static constexpr const char* CUTSCENE_MP4 = "videos/Cutscene_Entering_Train.mp4";
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
  // VideoPlayer is primarily MPEG-1 based; the repo includes a converter script:
  //   ./convert_videos.ps1  (requires ffmpeg)
  // It converts Assets/videos/*.mp4 -> *.mpg for reliable playback.

  bool loaded = false;

  if (player.open(CUTSCENE_MPG)) {
    loaded = true;
  }
  else if (player.open(CUTSCENE_MP4)) {
    loaded = true;
  }

  if (!loaded) {
    Log::error("[EnteringTrainCutsceneState] Failed to load cutscene: {} (or fallback {})", CUTSCENE_MPG, CUTSCENE_MP4);
    Log::error("[EnteringTrainCutsceneState] If using .mp4, convert to .mpg with: ./convert_videos.ps1 (ffmpeg required)");

    Engine::EventManager::send_event<Engine::Events::ToastEvent>(
      "Cutscene failed to load. Convert mp4 to mpg (convert_videos.ps1)",
      Engine::Events::ToastType::Warning,
      5.0f
    );

    Engine::SceneManager::pop_scene();
    return;
  }

  player.set_volume(1.0f);
  player.play();
}

void EnteringTrainCutsceneState::update()
{
  auto finish_cutscene = [this]() {
    player.stop();

    // Return to the underlying scene (e.g. GameplayState). The level was already opened
    // before this cutscene was pushed.
    Engine::SceneManager::pop_scene();
  };

  // Allow skipping the cutscene with Escape or Space
  if (Engine::Input::is_pressed(Engine::KeyCode::Escape) ||
      Engine::Input::is_pressed(Engine::KeyCode::Space))
  {
    finish_cutscene();
    return;
  }

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
