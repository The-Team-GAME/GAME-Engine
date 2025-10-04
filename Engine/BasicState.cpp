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
#include "core.h"
#include "InputManager.h"
#include <glm/glm.hpp>
#include <iostream> 
#include <GLFW/glfw3.h>
#include <imgui.h>

void BasicState::load()
{
  // Load fonts using ResourceManager
  font_id_1 = Engine::RESOURCES.load_font("roboto", "Assets/fonts/Roboto-Regular.ttf", 48);
  if (font_id_1 == -1)
  {
    std::cout << "Failed to load roboto font for BasicState.\n";
  }

  font_id_2 = Engine::RESOURCES.load_font("playfair", "Assets/fonts/PlayfairDisplay-VariableFont_wght.ttf", 48);
  if (font_id_2 == -1)
  {
    std::cout << "Failed to load playfair font for BasicState.\n";
  }

}

void BasicState::init()
{

}

void BasicState::update()
{
  rotation += 45.0f * Engine::Time::get_fixed_dt();
}

void BasicState::draw()
{
  Engine::GRAPHICS.clear_screen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background


  Engine::GRAPHICS.draw_rect({ 1.0f, 0.0f, 0.0f, 1.0f }, 0.0f, 0.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);        // Top left, Red
  Engine::GRAPHICS.draw_rect({ 0.0f, 1.0f, 0.0f, 1.0f }, 1600.0f, 900.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);   // Bottom right, Green
  Engine::GRAPHICS.draw_rect({ 0.0f, 0.0f, 1.0f, 1.0f }, 0.0f, 900.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);      // Bottom left, Blue
  Engine::GRAPHICS.draw_rect({ 1.0f, 1.0f, 0.0f, 1.0f }, 1600.0f, 0.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);     // Top right, Yellow
  Engine::GRAPHICS.draw_rect({ 1.0f, 0.0f, 1.0f, 1.0f }, 800.0f, 450.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);    // Center, Magenta

  Engine::GRAPHICS.draw_circle({ 0.0f, 1.0f, 1.0f, 1.0f }, 400.0f, 300.0f, 25.0f); // Cyan circle
  Engine::GRAPHICS.draw_line({ 1.0f, 1.0f, 1.0f, 1.0f }, 200.0f, 200.0f, 600.0f, 400.0f, 5.0f); // White line
  Engine::GRAPHICS.draw_line({ 1.0f, 1.0f, 1.0f, 1.0f }, 800.0f, 200.0f, 600.0f, 400.0f, 1.0f); // White line

  // Draw some text using the loaded font
  Engine::GRAPHICS.draw_text(font_id_1, "Hello", 250.0f, 250.0f, 1.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
  Engine::GRAPHICS.draw_text(font_id_2, "Hello", 250.0f, 300.0f, 1.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
  Engine::GRAPHICS.draw_text(font_id_1, "Press 'P' to play sound", 250.0f, 350.0f, 0.5f, { 1.0f, 1.0f, 0.0f, 1.0f });
}

void BasicState::free()
{

}

void BasicState::unload()
{
  // Unload resources using ResourceManager
  Engine::RESOURCES.unload_font("roboto");
  Engine::RESOURCES.unload_font("playfair");
  Engine::RESOURCES.unload_sound("move_sound");
}