//-----------------------------------------------------------------------------------
// file:    OpeningCutsceneState.cpp
// project: Monkey Stole My GPA
//
// brief:   Implementation of the opening cutscene state that plays before gameplay.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "OpeningCutsceneState.h"

void OpeningCutsceneState::load()
{
}

void OpeningCutsceneState::init()
{
  // Try to load the cutscene video
  // Note: VideoPlayer only supports .mpg (MPEG-1) format
  // If you have a .mp4 file, convert it using:
  // ffmpeg -i OpeningCutsceneWithSoundReduced.mp4 -c:v mpeg1video -c:a mp2 -f mpeg OpeningCutsceneWithSoundReduced.mpg

  bool loaded = false;

  // First try .mpg version (required format)
  if (player.open("videos/OpeningCutsceneWithSoundReduced.mpg"))
  {
    loaded = true;
  }
  // Fallback: try .mp4 (will likely fail, but attempt anyway)
  else if (player.open("videos/OpeningCutsceneWithSoundReduced.mp4"))
  {
    loaded = true;
  }

  if (!loaded)
  {
    // If video fails to load, skip directly to gameplay
    Log::error("[OpeningCutsceneState] Failed to load cutscene. Skipping to gameplay.");
    Log::error("[OpeningCutsceneState] Convert video to .mpg format with:");
    Log::error("[OpeningCutsceneState] ffmpeg -i OpeningCutsceneWithSoundReduced.mp4 -c:v mpeg1video -c:a mp2 -f mpeg OpeningCutsceneWithSoundReduced.mpg");
    Engine::SceneManager::load_scene("GameplayState");
    return;
  }

  player.set_volume(1.0f);
  player.play();
}

void OpeningCutsceneState::update()
{
  // Allow skipping the cutscene with Escape or Space
  if (Engine::Input::is_pressed(Engine::KeyCode::Escape) ||
    Engine::Input::is_pressed(Engine::KeyCode::Space))
  {
    player.stop();
    Engine::SceneManager::load_scene("GameplayState");
    return;
  }

  // Auto-transition to gameplay when video ends
  if (player.has_ended())
  {
    player.stop();
    Engine::SceneManager::load_scene("GameplayState");
    return;
  }
}

void OpeningCutsceneState::draw()
{
  // Clear screen to black
  Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });

  // Render the video centered in the virtual resolution
  Math::Vec2D vres = Engine::Graphics::get_virtual_resolution();
  player.render(vres.x, vres.y);
}

void OpeningCutsceneState::free()
{
  player.stop();
}

void OpeningCutsceneState::unload()
{
  player.close();
}
