//-----------------------------------------------------------------------------------
// file:    OptionsState.cpp
// project: Monkey Stole My GPA
//
// brief:   Options menu with BGM/SFX volume sliders and screen mode toggle.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "OptionsState.h"

// Static members
Engine::ECS::EntityID OptionsState::fullscreen_btn_id = 0;
Engine::ECS::EntityID OptionsState::windowed_btn_id = 0;
Engine::ECS::EntityID OptionsState::screen_mode_visual_id = 0;
Engine::Graphics::Texture OptionsState::fullscreen_tex;
Engine::Graphics::Texture OptionsState::windowed_tex;

// All entities created by this state, for easy cleanup
namespace {
  std::vector<Engine::ECS::EntityID> options_entities;

  void track(Engine::ECS::Entity e) {
    options_entities.push_back(e.get_id());
  }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
void OptionsState::update_slider_texture(Engine::ECS::Entity entity, int level)
{
  if (level < 1)  level = 1;
  if (level > 10) level = 10;
  auto* tex = entity.get_component<Components::Texture>();
  if (tex) {
    *tex = Components::Texture(slider_textures[level - 1]);
  }
}

bool OptionsState::is_mouse_in_rect(float cx, float cy, float w, float h)
{
  auto mouse_opt = Engine::Input::get_mouse_position_if_in_viewport();
  if (!mouse_opt.has_value()) return false;
  const auto& mouse = mouse_opt.value();
  float half_w = w * 0.5f;
  float half_h = h * 0.5f;
  return (mouse.x >= cx - half_w && mouse.x <= cx + half_w &&
    mouse.y >= cy - half_h && mouse.y <= cy + half_h);
}

int OptionsState::get_slider_level_from_mouse(float slider_cx, float slider_width)
{
  auto mouse_opt = Engine::Input::get_mouse_position_if_in_viewport();
  if (!mouse_opt.has_value()) return -1;
  float mx = mouse_opt.value().x;
  // slider left edge to right edge
  float left = slider_cx - slider_width * 0.5f;
  float right = slider_cx + slider_width * 0.5f;
  if (mx < left) mx = left;
  if (mx > right) mx = right;
  float t = (mx - left) / (right - left); // 0..1
  int level = static_cast<int>(t * 10.0f) + 1;
  if (level < 1)  level = 1;
  if (level > 10) level = 10;
  return level;
}

// ---------------------------------------------------------------------------
// Scene lifecycle
// ---------------------------------------------------------------------------
void OptionsState::load()
{
  // Load slider textures (1-10)
  for (int i = 0; i < 10; ++i) {
    std::string path = "images/Options/" + std::to_string(i + 1) + ".png";
    slider_textures[i] = Engine::Resources::load_texture(path);
  }

  // Load other textures
  fullscreen_tex = Engine::Resources::load_texture("images/Options/Fullscreen.png");
  windowed_tex = Engine::Resources::load_texture("images/Options/Windowed.png");

  // Read current settings to initialise slider positions
  auto& settings = Engine::FileIO::GameSettingsBridge::get();
  bgm_level = static_cast<int>(settings.bgm_volume * 10.0f + 0.5f);
  sfx_level = static_cast<int>(settings.sfx_volume * 10.0f + 0.5f);
  if (bgm_level < 1)  bgm_level = 1;
  if (bgm_level > 10) bgm_level = 10;
  if (sfx_level < 1)  sfx_level = 1;
  if (sfx_level > 10) sfx_level = 10;

  // -----------------------------------------------------------------------
  // Layout constants (virtual resolution 1600x900)
  // -----------------------------------------------------------------------
  // The panel image is the large pink "OPTIONS" box, displayed over the train
  // We centre it roughly in the screen.
  const float panel_w = 1406.0f;
  const float panel_h = 830.0f;
  const float panel_x = 760.0f;  // centre of 1600
  const float panel_y = 446 .0f;  // slightly above centre

  // Content area inside the panel — offset from the panel's top-left area
  // We'll lay things out row by row from the top of the content area.
  const float content_left = 480.0f;   // x where labels start
  const float slider_x = 900.0f;   // centre-x of sliders
  const float slider_w = 529.0f;   // slider image width (actual png size)
  const float slider_h = 24.0f;    // slider image height (actual png size)
  const float slider_scale = 1.0f;     // display at native size

  const float bgm_label_w = 117.0f;     // actual BGM.png size
  const float bgm_label_h = 47.0f;
  const float sfx_label_w = 103.0f;     // actual SFX.png size
  const float sfx_label_h = 48.0f;

  const float row1_y = 320.0f;          // BGM row
  const float row2_y = 400.0f;          // SFX row
  const float row3_y = 500.0f;          // SCREEN row

  const float screen_label_w = 211.0f;
  const float screen_label_h = 50.0f;

  const float mode_btn_w = 427.0f;
  const float mode_btn_h = 32.0f;
  const float mode_btn_x = 920.0f;    // centre of the fullscreen/windowed button area

  // Check current screen mode for initial texture
  const bool is_fullscreen = settings.fullscreen;

  // -----------------------------------------------------------------------
  // Create entities
  // -----------------------------------------------------------------------

  // --- Background (replicated from MainMenuState) ---
  // BackSky (scrolling) - layer 0
  backsky_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      Math::Vec2D(2400.0f, 450.0f), 0.0f, Math::Vec2D(4800.0f, 900.0f))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::Texture>(
      Engine::Resources::load_texture("images/UI/MainMenu/BackGround/BackSky.png", true, true))
    .add_to_layer("main_menu_0");
  track(backsky_entity);

  // BackCarriage - layer 2
  carriage_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::Texture>(
      Engine::Resources::load_texture("images/UI/MainMenu (Start)/BackCarriage.png", true, true))
    .add_to_layer("main_menu_2");
  track(carriage_entity);

  // Front overlay - layer 4
  front_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::Texture>(
      Engine::Resources::load_texture("images/UI/MainMenu (Start)/Front - no graffiti.png", true, true))
    .add_to_layer("main_menu_4");
  track(front_entity);

  // Panel (the big pink "OPTIONS" box) — drawn on the options layer (above background)
  panel_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      Math::Vec2D(panel_x, panel_y), 0.0f, Math::Vec2D(panel_w, panel_h))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::Texture>(
      Engine::Resources::load_texture("images/Options/New group.png"))
    .add_component<Components::UI>()
    .add_to_layer("options");
  track(panel_entity);

  // --- BGM row ---
  bgm_label_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      Math::Vec2D(content_left, row1_y), 0.0f, Math::Vec2D(bgm_label_w, bgm_label_h))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::Texture>(
      Engine::Resources::load_texture("images/Options/BGM.png"))
    .add_component<Components::UI>()
    .add_to_layer("options");
  track(bgm_label_entity);

  bgm_slider_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      Math::Vec2D(slider_x, row1_y), 0.0f,
      Math::Vec2D(slider_w * slider_scale, slider_h * slider_scale))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::Texture>(slider_textures[bgm_level - 1])
    .add_component<Components::UI>()
    .add_to_layer("options");
  track(bgm_slider_entity);

  // --- SFX row ---
  sfx_label_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      Math::Vec2D(content_left, row2_y), 0.0f, Math::Vec2D(sfx_label_w, sfx_label_h))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::Texture>(
      Engine::Resources::load_texture("images/Options/SFX.png"))
    .add_component<Components::UI>()
    .add_to_layer("options");
  track(sfx_label_entity);

  sfx_slider_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      Math::Vec2D(slider_x, row2_y), 0.0f,
      Math::Vec2D(slider_w * slider_scale, slider_h * slider_scale))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::Texture>(slider_textures[sfx_level - 1])
    .add_component<Components::UI>()
    .add_to_layer("options");
  track(sfx_slider_entity);

  // --- SCREEN row ---
  screen_label_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      Math::Vec2D(content_left, row3_y), 0.0f,
      Math::Vec2D(screen_label_w, screen_label_h))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::Texture>(
      Engine::Resources::load_texture("images/Options/SCREEN.png"))
    .add_component<Components::UI>()
    .add_to_layer("options");
  track(screen_label_entity);

  // Screen mode visual — displays Fullscreen.png or Windowed.png based on current setting
  screen_mode_visual_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      Math::Vec2D(mode_btn_x, row3_y), 0.0f,
      Math::Vec2D(mode_btn_w, mode_btn_h))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::Texture>(is_fullscreen ? fullscreen_tex : windowed_tex)
    .add_component<Components::UI>()
    .add_to_layer("options");
  track(screen_mode_visual_entity);
  screen_mode_visual_id = screen_mode_visual_entity.get_id();

  // Screen mode colliders — split into left (fullscreen) and right (windowed) halves
  const float half_btn_w = mode_btn_w * 0.5f;  // 213.5

  // Fullscreen button — left half collider, full-size texture
  fullscreen_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      Math::Vec2D(mode_btn_x - half_btn_w * 0.5f, row3_y), 0.0f,
      Math::Vec2D(half_btn_w, mode_btn_h))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::UI>()
    .add_component<Components::BoxCollider>(half_btn_w, mode_btn_h, false)
    .add_component<Components::Clickable>("options_set_fullscreen", "", "")
    .add_to_layer("options");
  track(fullscreen_entity);
  fullscreen_btn_id = fullscreen_entity.get_id();

  // Windowed button — right half collider, full-size texture
  windowed_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      Math::Vec2D(mode_btn_x + half_btn_w * 0.5f, row3_y), 0.0f,
      Math::Vec2D(half_btn_w, mode_btn_h))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::UI>()
    .add_component<Components::BoxCollider>(half_btn_w, mode_btn_h, false)
    .add_component<Components::Clickable>("options_set_windowed", "", "")
    .add_to_layer("options");
  track(windowed_entity);
  windowed_btn_id = windowed_entity.get_id();
}

void OptionsState::init()
{
}

void OptionsState::update()
{
  // Close on Escape — go back to main menu (this is a full scene, not an overlay)
  if (Engine::Input::is_pressed(Engine::KeyCode::Escape))
  {
    Engine::SceneManager::load_scene("MainMenuState");
    return;
  }

  // --- Sky scrolling animation (same as MainMenuState) ---
  float dt = Engine::Time::get_fixed_dt();
  sky_timer += dt;
  if (sky_timer >= sky_duration) {
    sky_timer -= sky_duration;
  }
  float sky_t = sky_timer / sky_duration;
  float sky_x = 2400.0f + ((-800.0f) - 2400.0f) * sky_t;

  if (backsky_entity.is_valid() && Engine::ECS::is_entity_active(backsky_entity)) {
    auto* transform = backsky_entity.get_component<Components::Transform2D>();
    if (transform) {
      transform->position.x = sky_x;
    }
  }

  // --- Slider interaction (manual mouse hit-testing) ---
  const float slider_x = 900.0f;
  const float slider_w = 529.0f;
  const float row1_y = 320.0f;
  const float row2_y = 400.0f;

  // Generous vertical hit area for easier clicking
  const float hit_h = 40.0f;

  if (Engine::Input::is_held(Engine::KeyCode::MouseLeft))
  {
    // BGM slider
    if (is_mouse_in_rect(slider_x, row1_y, slider_w, hit_h))
    {
      int new_level = get_slider_level_from_mouse(slider_x, slider_w);
      if (new_level != bgm_level && new_level >= 1 && new_level <= 10)
      {
        bgm_level = new_level;
        update_slider_texture(bgm_slider_entity, bgm_level);

        float volume = static_cast<float>(bgm_level) / 10.0f;
        Engine::AudioSystem::setBGMVolume(volume);

        auto& settings = Engine::FileIO::GameSettingsBridge::get_mutable();
        settings.bgm_volume = volume;
        Engine::FileIO::GameSettingsBridge::save();
      }
    }

    // SFX slider
    if (is_mouse_in_rect(slider_x, row2_y, slider_w, hit_h))
    {
      int new_level = get_slider_level_from_mouse(slider_x, slider_w);
      if (new_level != sfx_level && new_level >= 1 && new_level <= 10)
      {
        sfx_level = new_level;
        update_slider_texture(sfx_slider_entity, sfx_level);

        float volume = static_cast<float>(sfx_level) / 10.0f;
        Engine::AudioSystem::setSFXVolume(volume);

        auto& settings = Engine::FileIO::GameSettingsBridge::get_mutable();
        settings.sfx_volume = volume;
        Engine::FileIO::GameSettingsBridge::save();
      }
    }
  }
}

void OptionsState::draw()
{
}

void OptionsState::free()
{
}

void OptionsState::unload()
{
  // Destroy all entities we created
  for (auto id : options_entities) {
    if (Engine::ECS::is_entity_active(Engine::ECS::Entity(id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(id));
    }
  }
  options_entities.clear();
  fullscreen_btn_id = 0;
  windowed_btn_id = 0;
  screen_mode_visual_id = 0;
}
