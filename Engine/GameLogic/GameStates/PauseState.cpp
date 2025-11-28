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
#include "PauseState.h"
#include "../../Engine/MonkeyEngine.h"
#include "Time.h"

namespace PauseTextures {
  nothing_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/NothingSelectedPausedMenu.png");
  resume_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/ResumeButton.png");
  restart_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/RestartButton.png");
  options_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/OptionsButtons.png");
  exit_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/ExitButton.png");
}

void PauseState::load()
{ 
  background_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      0.0f, 0.0f,
      0.0f,
      1600.0f, 900.0f
    )
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture());
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);

  hovered_button_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      0.0f, 0.0f,
      0.0f,
      1600.0f, 900.0f
    )
    .add_component<Components::Texture>(PauseTextures::nothing_button_hovered);
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);
}

void PauseState::init()
{
}

void PauseState::update()
{ 
}

void PauseState::draw()
{ 
}

void PauseState::free()
{ 
}

void PauseState::unload()
{ 
}