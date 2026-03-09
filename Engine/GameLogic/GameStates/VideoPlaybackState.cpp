#include "pch.h"
#include "VideoPlaybackState.h"

void VideoPlaybackState::load()
{
}

void VideoPlaybackState::init()
{
  player.open("videos/output.mpg");
  player.set_volume(1.0f);
  player.play();
}

void VideoPlaybackState::update()
{
  accumulated_dt += Engine::Time::get_fixed_dt();

  // Press Escape to stop and go back
  if (Engine::Input::is_pressed(Engine::KeyCode::Escape))
  {
    player.stop();
    Engine::SceneManager::pop_scene();
    return;
  }

  // Space to toggle pause
  if (Engine::Input::is_pressed(Engine::KeyCode::Space))
  {
    player.pause(!player.is_paused());
  }

  // Auto-stop when video ends
  if (player.has_ended())
  {
    player.stop();
  }
}

void VideoPlaybackState::draw()
{
  // Advance video once per render frame (not per fixed timestep)
  if (accumulated_dt > 0.0f)
  {
    player.update(accumulated_dt);
    accumulated_dt = 0.0f;
  }

  Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });

  Math::Vec2D vres = Engine::Graphics::get_virtual_resolution();
  player.render(vres.x, vres.y);
}

void VideoPlaybackState::free()
{
  player.stop();
}

void VideoPlaybackState::unload()
{
  player.close();
}
