//-----------------------------------------------------------------------------------
// file:    OptionsState.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "OptionsState.h"
#include "../../Engine/MonkeyEngine.h"
#include "../../Engine/Engine/Systems/GraphicsSystem.h"

namespace {
  Engine::Graphics::Texture backsky_texture;
  Engine::Graphics::Texture carriage_texture;
  Engine::Graphics::Texture options_base_texture;

  Engine::ECS::Entity backsky_entity;
  Engine::ECS::Entity carriage_entity;
  Engine::ECS::Entity options_base_entity;
}

void OptionsState::load()
{
  backsky_texture = Engine::Resources::load_texture("images/UI/MainMenu/BackGround/BackSky.png");
  carriage_texture = Engine::Resources::load_texture("images/UI/MainMenu/BackGround/Carriage.png");
  options_base_texture = Engine::Resources::load_texture("images/UI/MainMenu(Options)/OptionsBase.png");

  backsky_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      800.0f, 450.0f,
      0.0f,
      1600.0f, 900.0f
    )
    .add_component<Components::Texture>(backsky_texture)
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_to_layer("background");

  carriage_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      800.0f, 450.0f,
      0.0f,
      1600.0f, 900.0f
    )
    .add_component<Components::Texture>(carriage_texture)
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_to_layer("effect"); // TODO: create a better layer for this

  options_base_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      800.0f, 450.0f,
      0.0f,
      1600.0f, 900.0f
    )
    .add_component<Components::Texture>(options_base_texture)
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_to_layer("ui");
}

void OptionsState::init()
{
}

void OptionsState::update()
{
}

void OptionsState::draw()
{
}

void OptionsState::free()
{
}

void OptionsState::unload()
{
}