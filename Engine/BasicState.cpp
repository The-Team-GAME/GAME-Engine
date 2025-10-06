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
#include "Components.h"
#include "Resources.h"
#include "RenderSystem.h"
#include "Time.h"
#include <glm/glm.hpp>
#include <iostream> 
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "Reflection/Helpers.h"

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

struct Book
{
  std::string title;
  std::string author;
  int pages;
};
REFLECT(Book, title, author, pages)

struct Person
{
  std::string name;
  int age;
  double height;
  bool is_student;
  Book favorite_book;
};
REFLECT(Person, name, age, height, is_student, favorite_book)

void BasicState::init()
{
  // Set resource path for loading assets
  Engine::Resources::set_resource_path("Assets/");

  // Add rendering systems
  ECS::World::add_system<RectRenderSystem>();
  ECS::World::add_system<TextRenderSystem>();

  // Create rectangle entities
  // Top left, Red
  rect_top_left = ECS::World::create_entity()
    .add_component<Components::Transform2D>(
      0.0f, 0.0f,
      rotation,
      50.0f, 50.0f
    )
    .add_component<Components::Color>(255.0f, 0.0f, 0.0f, 255.0f);

  // Bottom right, Green
  rect_bottom_right = ECS::World::create_entity()
    .add_component<Components::Transform2D>(
      1600.0f, 900.0f,
      rotation,
      50.0f, 50.0f
    ).add_component<Components::Color>(0.0f, 255.0f, 0.0f, 255.0f);

  // Bottom left, Blue
  rect_bottom_left = ECS::World::create_entity()
    .add_component<Components::Transform2D>(
      0.0f, 900.0f,
      rotation,
      50.0f, 50.0f
    ).add_component<Components::Color>(0.0f, 0.0f, 255.0f, 255.0f);

  // Top right, Yellow
  rect_top_right = ECS::World::create_entity()
    .add_component<Components::Transform2D>(
      1600.0f, 0.0f,
      rotation,
      50.0f, 50.0f
    ).add_component<Components::Color>(255.0f, 255.0f, 0.0f, 255.0f);

  // Center, Magenta
  rect_center = ECS::World::create_entity()
    .add_component<Components::Transform2D>(
      800.0f, 450.0f,
      rotation,
      50.0f, 50.0f
    ).add_component<Components::Color>(255.0f, 0.0f, 255.0f, 255.0f);

  // Create text entities
  text_hello_1 = ECS::World::create_entity()
    .add_component<Components::Transform2D>(
      250.0f, 250.0f,
      0.0f,
      1.0f, 1.0f
    )
    .add_component<Components::Font>(font_roboto)
    .add_component<Components::Text>("Hello");

  text_hello_2 = ECS::World::create_entity()
    .add_component<Components::Transform2D>(
      250.0f, 300.0f,
      0.0f,
      1.0f, 1.0f
    )
    .add_component<Components::Font>(font_playfair)
    .add_component<Components::Text>("Hello");

  text_instruction = ECS::World::create_entity()
    .add_component<Components::Transform2D>(
      250.0f, 350.0f,
      0.0f,
      0.5f, 0.5f
    )
    .add_component<Components::Font>(font_roboto)
    .add_component<Components::Text>("Press 'S' to play sound");


  // Trying reflection system

  // auto col1 = rect_center.get_component<Components::Color>();

  // // Serialize 
  // std::string json = serialize_to_json(*col1);
  // std::cout << "Serialized: " << json << "\n\n";

  // // Deserialize
  // Components::Color col2;
  // if (deserialize_from_json(col2, json))
  // {
  //   std::cout << "Deserialized successfully:\n";
  //   print_struct_fields(col2);
  // }
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
  Engine::Graphics::clear_screen({ 20.0f, 20.0f, 20.0f, 255.0f });

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
    { 255.0f, 255.0f, 255.0f, 255.0f }
  );

  static auto move_sound = Engine::Resources::load_sound("sounds/move1.wav");

  if (Engine::Input::is_key_pressed(GLFW_KEY_S)) {
    Engine::Audio::play_sound(move_sound, 0.5f, false);
  }


  ImGui::Begin("Reflection Demo");

  auto col1 = rect_center.get_component<Components::Color>();
  if (col1)
  {
    if (ImGui::CollapsingHeader("Center Rectangle Color"))
    {
      display_struct_in_imgui(*col1);
    }
  }

  auto transform = rect_center.get_component<Components::Transform2D>();
  if (transform)
  {
    if (ImGui::CollapsingHeader("Center Rectangle Transform"))
    {
      display_struct_in_imgui(*transform);
    }
  }

  auto text = text_instruction.get_component<Components::Text>();
  if (text)
  {
    if (ImGui::CollapsingHeader("Instruction Text"))
    {
      display_struct_in_imgui(*text);
    }
  }

  ImGui::End();
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
  ECS::World::remove_system<RectRenderSystem>();
  ECS::World::remove_system<TextRenderSystem>();
}

void BasicState::unload()
{
  // Reset font pointers (actual cleanup handled by resource manager)
  font_roboto.reset();
  font_playfair.reset();

  // Clear unused resources from cache
  Engine::Resources::clear_unused_resources();
}