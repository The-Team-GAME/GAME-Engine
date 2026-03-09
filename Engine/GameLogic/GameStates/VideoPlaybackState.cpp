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
  f32 dt = Engine::Time::get_fixed_dt();
  player.update(dt);

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
  Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });

  auto tex = player.get_texture();
  if (tex)
  {
    // Center the video on screen, fitting to virtual resolution
    Math::Vec2D vres = Engine::Graphics::get_virtual_resolution();
    f32 vid_w = static_cast<f32>(player.get_width());
    f32 vid_h = static_cast<f32>(player.get_height());

    // Scale to fit while preserving aspect ratio
    f32 scale = (std::min)(vres.x / vid_w, vres.y / vid_h);
    Math::Vec2D size{ vid_w * scale, vid_h * scale };
    Math::Vec2D pos{ vres.x * 0.5f, vres.y * 0.5f };

    Engine::Graphics::draw_texture_ui(tex, pos, size, 0.0f,
      { 1.0f, 1.0f, 1.0f, 1.0f });
  }
}

void VideoPlaybackState::free()
{
  player.stop();
}

void VideoPlaybackState::unload()
{
  player.close();
}
