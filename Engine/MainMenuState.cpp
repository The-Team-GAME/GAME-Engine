#include "MainMenuState.h"
#include "core.h"
#include "InputManager.h"
#include <glm/glm.hpp>
#include <iostream> 
#include <GLFW/glfw3.h>

void MainMenuState::Load()
{
}

void MainMenuState::Init()
{
}

void MainMenuState::Update()
{
}

void MainMenuState::Draw()
{
  Engine::GRAPHICS->clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background

  Engine::GRAPHICS->drawRect({ 1.0f, 0.0f, 0.0f, 1.0f }, 100.0f, 100.0f, 200.0f, 50.0f);  // Red
}

void MainMenuState::Free()
{
}

void MainMenuState::Unload()
{
}