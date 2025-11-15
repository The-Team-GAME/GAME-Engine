//-----------------------------------------------------------------------------------
// file:    BasicState.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the BasicState which demonstrates basic rendering
//          capabilities including shapes (rectangles, circles, lines), text rendering
//          with multiple fonts, and audio playback.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "BasicState.h"
#include "../../Engine/MonkeyEngine.h"
#include "Time.h"
#include <iostream>
#include <imgui.h>

void BasicState::load()
{
  // Load fonts using the new resource manager
  font_roboto = Engine::Resources::load_font("fonts/Roboto-Regular.ttf", 48);
  if (!font_roboto)
  {
    std::cout << "Failed to load roboto font for BasicState.\n";
  }

  font_playfair = Engine::Resources::load_font("fonts/PlayfairDisplay-VariableFont_wght.ttf", 48);
  if (!font_playfair)
  {
    std::cout << "Failed to load playfair font for BasicState.\n";
  }
}

void BasicState::init()
{

  // Create rectangle entities
  // Top left, Red
  rect_top_left = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      0.0f, 0.0f,
      rotation,
      50.0f, 50.0f
    )
    .add_component<Components::Color>(1.0f, 0.0f, 0.0f, 1.0f)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture());

  // Bottom right, Green
  rect_bottom_right = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      1600.0f, 900.0f,
      rotation,
      50.0f, 50.0f
    )
    .add_component<Components::Color>(0.0f, 1.0f, 0.0f, 1.0f)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture());

  // Bottom left, Blue
  rect_bottom_left = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      0.0f, 900.0f,
      rotation,
      50.0f, 50.0f
    )
    .add_component<Components::Color>(0.0f, 0.0f, 1.0f, 1.0f)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture());

  // Top right, Yellow
  rect_top_right = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      1600.0f, 0.0f,
      rotation,
      50.0f, 50.0f
    )
    .add_component<Components::Color>(1.0f, 1.0f, 0.0f, 1.0f)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture());

  // Center, Magenta
  rect_center = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      800.0f, 450.0f,
      rotation,
      50.0f, 50.0f
    )
    .add_component<Components::Color>(1.0f, 0.0f, 1.0f, 1.0f)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture());

  // Create text entities
  text_hello_1 = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      250.0f, 250.0f,
      0.0f,
      1.0f, 1.0f
    )
    .add_component<Components::Font>(font_roboto)
    .add_component<Components::Text>("Hello");

  text_hello_2 = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      250.0f, 300.0f,
      0.0f,
      1.0f, 1.0f
    )
    .add_component<Components::Font>(font_playfair)
    .add_component<Components::Text>("Hello");

  text_instruction = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      250.0f, 350.0f,
      0.0f,
      0.5f, 0.5f
    )
    .add_component<Components::Font>(font_roboto)
    .add_component<Components::Text>("Press 'S' to play sound");

}

void BasicState::update()
{
  rotation += 45.0f * Engine::Time::get_fixed_dt();

  // Update rotation for all rectangle entities
  rect_top_left.get_component<Components::Transform2D>()->rotation = rotation;
  rect_bottom_right.get_component<Components::Transform2D>()->rotation = rotation;
  rect_bottom_left.get_component<Components::Transform2D>()->rotation = rotation;
  rect_top_right.get_component<Components::Transform2D>()->rotation = rotation;
  rect_center.get_component<Components::Transform2D>()->rotation = rotation;
}

void BasicState::draw()
{
  // Clear background
  Engine::Graphics::clear_screen({ 0.078f, 0.078f, 0.078f, 1.0f });

  auto unit_rect = Engine::Resources::get_rect_texture();

  Engine::Graphics::draw_texture(
    unit_rect,
    { 400.0f - 25.0f, 300.0f - 25.0f },
    { 50.0f, 50.0f },
    0.0f,
    { 0.0f, 1.0f, 1.0f, 1.0f }
  );

  Engine::Graphics::draw_text(
    font_roboto,
    "Monkey Stole My GPA",
    { 400.0f, 100.0f },
    1.0f,
    { 1.0f, 1.0f, 1.0f, 1.0f }
  );

  Engine::Graphics::draw_line(
    { 300.0f, 400.0f },
    { 500.0f, 400.0f },
    5.0f,
    { 1.0f, 0.5f, 0.0f, 1.0f }
  );

  Engine::Graphics::draw_rect_outline(
    { 400.0f, 500.0f },
    { 100.0f, 50.0f },
    30.0f,
    3.0f,
    { 0.5f, 0.0f, 0.5f, 1.0f }
  );

  // Draw a triangle (equilateral)
  Engine::Graphics::draw_triangle(
    { 600.0f, 450.0f },   // position
    40.0f,                // radius
    rotation,             // rotation (animated)
    { 1.0f, 0.5f, 0.0f, 1.0f }  // orange color
  );

  // Draw a circle with default segments (32)
  Engine::Graphics::draw_circle(
    { 700.0f, 450.0f },   // position
    35.0f,                // radius
    { 0.0f, 1.0f, 0.5f, 1.0f }  // cyan color
  );

  // Draw a circle with more segments for smoother appearance
  Engine::Graphics::draw_circle(
    { 800.0f, 450.0f },   // position
    35.0f,                // radius
    { 0.5f, 0.0f, 1.0f, 1.0f },  // purple color
    64                    // more segments = smoother
  );

  // Draw a custom polygon (hexagon)
  {
    std::vector<Math::Vec2D> hexagon_vertices;
    for (int i = 0; i < 6; ++i) {
      float angle = i * (360.0f / 6.0f) * 3.14159265359f / 180.0f;
      hexagon_vertices.push_back(Math::Vec2D(std::cos(angle) * 40.0f, std::sin(angle) * 40.0f));
    }
    Engine::Graphics::draw_polygon(
      hexagon_vertices,
      { 900.0f, 450.0f },  // position
      rotation * 0.5f,     // slower rotation
      { 1.0f, 1.0f, 0.0f, 1.0f }  // yellow color
    );
  }

  // Draw a custom polygon (star shape)
  {
    std::vector<Math::Vec2D> star_vertices;
    int points = 5;
    float outer_radius = 40.0f;
    float inner_radius = 18.0f;
    for (int i = 0; i < points * 2; ++i) {
      float angle = i * (360.0f / (points * 2.0f)) * 3.14159265359f / 180.0f - 3.14159265359f / 2.0f;
      float radius = (i % 2 == 0) ? outer_radius : inner_radius;
      star_vertices.push_back(Math::Vec2D(std::cos(angle) * radius, std::sin(angle) * radius));
    }
    Engine::Graphics::draw_polygon(
      star_vertices,
      { 1000.0f, 450.0f }, // position
      -rotation,           // reverse rotation
      { 1.0f, 0.8f, 0.0f, 1.0f }  // gold color
    );
  }

  // Draw outline shapes below
  // Triangle outline
  Engine::Graphics::draw_triangle_outline(
    { 600.0f, 600.0f },   // position
    40.0f,                // radius
    rotation,             // rotation (animated)
    3.0f,                 // thickness
    { 1.0f, 0.5f, 0.0f, 1.0f }  // orange color
  );

  // Circle outline
  Engine::Graphics::draw_circle_outline(
    { 700.0f, 600.0f },   // position
    35.0f,                // radius
    2.5f,                 // thickness
    { 0.0f, 1.0f, 0.5f, 1.0f },  // cyan color
    32                    // segments
  );

  // Hexagon outline
  {
    std::vector<Math::Vec2D> hexagon_vertices;
    for (int i = 0; i < 6; ++i) {
      float angle = i * (360.0f / 6.0f) * 3.14159265359f / 180.0f;
      hexagon_vertices.push_back(Math::Vec2D(std::cos(angle) * 40.0f, std::sin(angle) * 40.0f));
    }
    Engine::Graphics::draw_polygon_outline(
      hexagon_vertices,
      { 800.0f, 600.0f },  // position
      rotation * 0.5f,     // slower rotation
      2.0f,                // thickness
      { 1.0f, 1.0f, 0.0f, 1.0f }  // yellow color
    );
  }

  // Star outline
  {
    std::vector<Math::Vec2D> star_vertices;
    int points = 5;
    float outer_radius = 40.0f;
    float inner_radius = 18.0f;
    for (int i = 0; i < points * 2; ++i) {
      float angle = i * (360.0f / (points * 2.0f)) * 3.14159265359f / 180.0f - 3.14159265359f / 2.0f;
      float radius = (i % 2 == 0) ? outer_radius : inner_radius;
      star_vertices.push_back(Math::Vec2D(std::cos(angle) * radius, std::sin(angle) * radius));
    }
    Engine::Graphics::draw_polygon_outline(
      star_vertices,
      { 900.0f, 600.0f },  // position
      -rotation,           // reverse rotation
      2.5f,                // thickness
      { 1.0f, 0.8f, 0.0f, 1.0f }  // gold color
    );
  }

  // Normally it would be a horrible idewa to load a texture every frame like this,
  // but for demonstration purposes, we will do it here to show that the resource
  // manager caches the texture and does not reload it from disk every time.
  // If you look at the memory usage, you will see that it does not increase !!
  //auto monkey_texture = Engine::Resources::load_texture("images/monkey.png");

  // nvm the constant printing pissed me off, so imma make it static
  static auto monkey_texture = Engine::Resources::load_texture("images/monkey.png");

  Engine::Graphics::draw_texture(
    monkey_texture,
    { 500.0f, 300.0f },
    { static_cast<f32>(monkey_texture->width * 10), static_cast<f32>(monkey_texture->height * 10) },
    0.0f,
    { 1.0f, 1.0f, 1.0f, 1.0f }
  );

  static auto move_sound = Engine::Resources::load_sound("sounds/move1.wav");

  if (Engine::Input::is_pressed(Engine::KeyCode::S)) {
    Engine::Audio::play_sound(move_sound, 0.5f, false);
  }
}

void BasicState::free()
{
  // Destroy all entities
  if (rect_top_left.is_valid()) {
    rect_top_left.destroy();
  }
  if (rect_bottom_right.is_valid()) {
    rect_bottom_right.destroy();
  }
  if (rect_bottom_left.is_valid()) {
    rect_bottom_left.destroy();
  }
  if (rect_top_right.is_valid()) {
    rect_top_right.destroy();
  }
  if (rect_center.is_valid()) {
    rect_center.destroy();
  }
  if (text_hello_1.is_valid()) {
    text_hello_1.destroy();
  }
  if (text_hello_2.is_valid()) {
    text_hello_2.destroy();
  }
  if (text_instruction.is_valid()) {
    text_instruction.destroy();
  }
}

void BasicState::unload()
{
  // Reset font pointers (actual cleanup handled by resource manager)
  font_roboto.reset();
  font_playfair.reset();

  // Clear unused resources from cache
  Engine::Resources::clear_unused_resources();
}