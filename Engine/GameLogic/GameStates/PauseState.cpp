//-----------------------------------------------------------------------------------
// file:    PauseState.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "PauseState.h"
#include "../../Engine/Engine/Systems/GraphicsSystem.h"
#include "../../Engine/Engine/Audio/AudioSystem.h"

// Static member definitions
Engine::ECS::EntityID PauseState::menu_sprite_id = 0;
int PauseState::hovered_button = 0;
Engine::Graphics::Texture PauseState::nothing_selected_tex;
Engine::Graphics::Texture PauseState::resume_selected_tex;
Engine::Graphics::Texture PauseState::restart_selected_tex;
Engine::Graphics::Texture PauseState::options_selected_tex;
Engine::Graphics::Texture PauseState::exit_selected_tex;

// Helper to update menu sprite texture
namespace {
  static constexpr const char* PAUSE_LOWPASS_SNAPSHOT_INSTANCE = "pause_lowpass_snapshot";
  static constexpr const char* PAUSE_LOWPASS_SNAPSHOT_EVENT = "snapshot:/LowpassSnapshot";
  static constexpr const char* PAUSE_BUTTON_HOVER_EVENT = "event:/SFXUI/ButtonHover";
  static constexpr const char* PAUSE_BUTTON_CLICK_EVENT = "event:/SFXUI/ButtonClick";
  static bool gPauseLowpassSnapshotActive = false;

  void set_pause_menu_texture(const Engine::Graphics::Texture& tex) {
    if (PauseState::menu_sprite_id == 0) return;
    if (!Engine::ECS::is_entity_active(Engine::ECS::Entity(PauseState::menu_sprite_id))) return;

    auto* texture_comp = Engine::ECS::Entity(PauseState::menu_sprite_id).get_component<Components::Texture>();
    if (texture_comp) {
      *texture_comp = tex;
    }
  }

  void start_pause_lowpass_snapshot() {
    if (gPauseLowpassSnapshotActive) return;

    if (!Engine::AudioSystem::createEventInstance(PAUSE_LOWPASS_SNAPSHOT_INSTANCE, PAUSE_LOWPASS_SNAPSHOT_EVENT)) {
      return;
    }

    if (!Engine::AudioSystem::startEventInstance(PAUSE_LOWPASS_SNAPSHOT_INSTANCE)) {
      Engine::AudioSystem::releaseEventInstance(PAUSE_LOWPASS_SNAPSHOT_INSTANCE);
      return;
    }

    gPauseLowpassSnapshotActive = true;
  }

  void stop_pause_lowpass_snapshot() {
    if (!gPauseLowpassSnapshotActive) return;

    Engine::AudioSystem::stopEventInstance(PAUSE_LOWPASS_SNAPSHOT_INSTANCE, false);
    Engine::AudioSystem::releaseEventInstance(PAUSE_LOWPASS_SNAPSHOT_INSTANCE);
    gPauseLowpassSnapshotActive = false;
  }
}

void PauseState::load()
{
  start_pause_lowpass_snapshot();

  // Load textures
  nothing_selected_tex = Engine::Resources::load_texture("images/UI/In-Game Pause/States/NothingSelectedPausedMenu.png");
  resume_selected_tex = Engine::Resources::load_texture("images/UI/In-Game Pause/States/ResumeButton.png");
  restart_selected_tex = Engine::Resources::load_texture("images/UI/In-Game Pause/States/RestartButton.png");
  options_selected_tex = Engine::Resources::load_texture("images/UI/In-Game Pause/States/OptionsButtons.png");
  exit_selected_tex = Engine::Resources::load_texture("images/UI/In-Game Pause/States/ExitButton.png");

  // Get current camera position to center the UI
  Components::Transform2D camera = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }; // Engine::Systems::CameraSystem::get_camera().position; --- IGNORE ---
  float cam_x = camera.position.x + 800.0f;
  float cam_y = camera.position.y + 450.0f;

  // Create black semi-transparent background covering whole screen
  auto background = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(cam_x, cam_y, 0.0f, 1600.0f, 900.0f)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
    .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 0.5f)
    .add_component<Components::UI>()
    .add_to_layer("main_menu_4");
  background_id = background.get_id();

  // Create menu sprite (starts with nothing selected)
  auto menu_sprite = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(cam_x, cam_y, 0.0f, 1600.0f, 900.0f)
    .add_component<Components::Texture>(nothing_selected_tex)
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::UI>()
    .add_to_layer("main_menu_5");
  menu_sprite_id = menu_sprite.get_id();

  // Helper lambda to create a polygon collider with proper offset
  auto create_polygon_collider = [](const std::vector<Math::Vec2D>& vertices, Math::Vec2D offset = Math::Vec2D(0.0f, 0.0f)) {
    Components::BoxCollider collider(vertices, false);
    collider.offset = offset;
    return collider;
    };

  // Resume button collider - uses Clickable with hover callbacks
  auto resume_collider = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(cam_x, cam_y, 0.0f, 1600.0f, 900.0f)
    .add_component<Components::BoxCollider>(create_polygon_collider(
      std::vector<Math::Vec2D>{
    Math::Vec2D(-504.8023986816406f, -354.3161315917969f),
      Math::Vec2D(-507.3556823730469f, -167.93307495117188f),
      Math::Vec2D(-51.975685119628906f, -165.8661346435547f),
      Math::Vec2D(-55.50151824951172f, -250.0f)
  }))
    .add_component<Components::Clickable>("pause_menu_resume", "pause_hover_resume", "pause_hover_none")
    .add_component<Components::UI>()
    .add_to_layer("main_menu_5");
  resume_collider_id = resume_collider.get_id();

  // Restart button collider
  auto restart_collider = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(cam_x, cam_y, 0.0f, 1600.0f, 900.0f)
    .add_component<Components::BoxCollider>(create_polygon_collider(
      std::vector<Math::Vec2D>{
    Math::Vec2D(-522.64453125f, -128.81448364257813f),
      Math::Vec2D(-525.1974487304688f, 15.015185356140137f),
      Math::Vec2D(-54.620059967041016f, -68.02435302734375f),
      Math::Vec2D(-58.14589309692383f, -158.23712158203125f)
  }))
    .add_component<Components::Clickable>("pause_menu_restart", "pause_hover_restart", "pause_hover_none")
    .add_component<Components::UI>()
    .add_to_layer("main_menu_5");
  restart_collider_id = restart_collider.get_id();

  // Options button collider
  auto options_collider = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(cam_x, cam_y, 0.0f, 1600.0f, 900.0f)
    .add_component<Components::BoxCollider>(create_polygon_collider(
      std::vector<Math::Vec2D>{
    Math::Vec2D(-516.1705932617188f, 209.14901733398438f),
      Math::Vec2D(-521.7630615234375f, 55.106380462646484f),
      Math::Vec2D(-57.264434814453125f, -43.13072204589844f),
      Math::Vec2D(-57.750755310058594f, 52.06686782836914f)
  }))
    .add_component<Components::Clickable>("pause_menu_options", "pause_hover_options", "pause_hover_none")
    .add_component<Components::UI>()
    .add_to_layer("main_menu_5");
  options_collider_id = options_collider.get_id();

  // Exit button collider
  auto exit_collider = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(cam_x, cam_y, 0.0f, 1600.0f, 900.0f)
    .add_component<Components::BoxCollider>(create_polygon_collider(
      std::vector<Math::Vec2D>{
    Math::Vec2D(-518.815185546875f, 386.018310546875f),
      Math::Vec2D(-527.4470825195313f, 244.1337432861328f),
      Math::Vec2D(-53.82978439331055f, 63.829795837402344f),
      Math::Vec2D(-54.31610107421875f, 152.9483184814453f)
  }))
    .add_component<Components::Clickable>("pause_menu_exit", "pause_hover_exit", "pause_hover_none")
    .add_component<Components::UI>()
    .add_to_layer("main_menu_5");
  exit_collider_id = exit_collider.get_id();

  Engine::Time::set_time_scale(0.0001f); // can't set to 0 or input will not work
}

void PauseState::init()
{
}

void PauseState::update()
{
  // Track previous hover state to detect when we stop hovering
  static int prev_hovered = 0;

  // Play hover SFX when cursor enters a different pause button.
  if (hovered_button != 0 && hovered_button != prev_hovered) {
    Engine::AudioSystem::playEvent(PAUSE_BUTTON_HOVER_EVENT);
  }

  // Play click SFX only when clicking while hovering a pause button.
  if (hovered_button != 0 && Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
    Engine::AudioSystem::playEvent(PAUSE_BUTTON_CLICK_EVENT);
  }

  // If we were hovering a button but now we're not, reset to nothing selected
  if (prev_hovered != 0 && hovered_button == 0) {
    if (menu_sprite_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(menu_sprite_id))) {
      auto* tex = Engine::ECS::Entity(menu_sprite_id).get_component<Components::Texture>();
      if (tex) *tex = nothing_selected_tex;
    }
  }
  prev_hovered = hovered_button;
  hovered_button = 0; // Reset each frame, on_hover will set it again if still hovering

  // Allow ESC to resume
  if (Engine::Input::is_pressed(Engine::KeyCode::Escape)) {
    Engine::SceneManager::pop_scene();
  }
}

void PauseState::draw()
{
}

void PauseState::free()
{
}

void PauseState::unload()
{
  stop_pause_lowpass_snapshot();

  // Destroy pause menu entities
  if (background_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(background_id))) {
    Engine::ECS::destroy_entity(Engine::ECS::Entity(background_id));
  }
  if (menu_sprite_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(menu_sprite_id))) {
    Engine::ECS::destroy_entity(Engine::ECS::Entity(menu_sprite_id));
  }
  if (resume_collider_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(resume_collider_id))) {
    Engine::ECS::destroy_entity(Engine::ECS::Entity(resume_collider_id));
  }
  if (restart_collider_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(restart_collider_id))) {
    Engine::ECS::destroy_entity(Engine::ECS::Entity(restart_collider_id));
  }
  if (options_collider_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(options_collider_id))) {
    Engine::ECS::destroy_entity(Engine::ECS::Entity(options_collider_id));
  }
  if (exit_collider_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(exit_collider_id))) {
    Engine::ECS::destroy_entity(Engine::ECS::Entity(exit_collider_id));
  }

  background_id = 0;
  menu_sprite_id = 0;
  resume_collider_id = 0;
  restart_collider_id = 0;
  options_collider_id = 0;
  exit_collider_id = 0;

  Engine::Time::set_time_scale(1.0f);
}