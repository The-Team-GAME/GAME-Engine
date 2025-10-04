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
#include "Core.h"
#include "InputManager.h"
#include "Graphics/Components.h"
#include "Graphics/Resource.h"
#include "RenderSystem.h"
#include "Time.h"
#include <glm/glm.hpp>
#include <iostream> 
#include <GLFW/glfw3.h>
#include <imgui.h>

void BasicState::load()
{
  // Load fonts using the new resource manager
  font_roboto = Engine::Resources::load_font("Assets/fonts/Roboto-Regular.ttf", 48);
  if (!font_roboto)
  {
    std::cout << "Failed to load roboto font for BasicState.\n";
  }

  font_playfair = Engine::Resources::load_font("Assets/fonts/PlayfairDisplay-VariableFont_wght.ttf", 48);
  if (!font_playfair)
  {
    std::cout << "Failed to load playfair font for BasicState.\n";
  }
}

void BasicState::init()
{
  // Add rendering systems
  Engine::WORLD.add_system<RectRenderSystem>();
  Engine::WORLD.add_system<TextRenderSystem>();

  // Create rectangle entities
  // Top left, Red
  rect_top_left = Engine::WORLD.create_entity()
    .add_component<Components::Transform2D>(
      0.0f, 0.0f,
      rotation,
      50.0f, 50.0f
    )
    .add_component<Components::Color>(255.0f, 0.0f, 0.0f, 255.0f);

  // Bottom right, Green
  rect_bottom_right = Engine::WORLD.create_entity()
    .add_component<Components::Transform2D>(
      1600.0f, 900.0f,
      rotation,
      50.0f, 50.0f
    ).add_component<Components::Color>(0.0f, 255.0f, 0.0f, 255.0f);

  // Bottom left, Blue
  rect_bottom_left = Engine::WORLD.create_entity()
    .add_component<Components::Transform2D>(
      0.0f, 900.0f,
      rotation,
      50.0f, 50.0f
    ).add_component<Components::Color>(0.0f, 0.0f, 255.0f, 255.0f);

  // Top right, Yellow
  rect_top_right = Engine::WORLD.create_entity()
    .add_component<Components::Transform2D>(
      1600.0f, 0.0f,
      rotation,
      50.0f, 50.0f
    ).add_component<Components::Color>(255.0f, 255.0f, 0.0f, 255.0f);

  // Center, Magenta
  rect_center = Engine::WORLD.create_entity()
    .add_component<Components::Transform2D>(
      800.0f, 450.0f,
      rotation,
      50.0f, 50.0f
    ).add_component<Components::Color>(255.0f, 0.0f, 255.0f, 255.0f);

  // Create text entities
  text_hello_1 = Engine::WORLD.create_entity()
    .add_component<Components::Transform2D>(
      250.0f, 250.0f,
      0.0f,
      1.0f, 1.0f
    )
    .add_component<Components::Font>(font_roboto)
    .add_component<Components::Text>("Hello");

  text_hello_2 = Engine::WORLD.create_entity()
    .add_component<Components::Transform2D>(
      250.0f, 300.0f,
      0.0f,
      1.0f, 1.0f
    )
    .add_component<Components::Font>(font_playfair)
    .add_component<Components::Text>("Hello");

  text_instruction = Engine::WORLD.create_entity()
    .add_component<Components::Transform2D>(
      250.0f, 350.0f,
      0.0f,
      0.5f, 0.5f
    )
    .add_component<Components::Font>(font_roboto)
    .add_component<Components::Text>("Press 'P' to play sound");
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
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  auto unit_rect = Engine::Resources::get_unit_rect_texture();

  Engine::Graphics::draw_texture(
    unit_rect,
    { 400.0f - 25.0f, 300.0f - 25.0f },
    { 50.0f, 50.0f },
    0.0f,
    { 0.0f, 255.0f, 255.0f, 255.0f }
  );

  Engine::Graphics::draw_text(
    font_roboto,
    "Monkey Stole My GPA",
    { 400.0f, 100.0f },
    1.0f,
    { 255.0f, 255.0f, 255.0f, 255.0f }
  );

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

  // Remove systems
  Engine::WORLD.remove_system<RectRenderSystem>();
  Engine::WORLD.remove_system<TextRenderSystem>();
}

void BasicState::unload()
{
  // Reset font pointers (actual cleanup handled by resource manager)
  font_roboto.reset();
  font_playfair.reset();

  // Clear unused resources from cache
  Engine::Resources::clear_unused_resources();
}