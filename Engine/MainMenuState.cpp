#include "MainMenuState.h"
#include "Graphics.h"
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
  Graphics::clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background

  Graphics::drawRect(100.0f, 100.0f, 200.0f, 50.0f, { 1.0f, 0.0f, 0.0f, 1.0f });  // Red
}

void MainMenuState::Free()
{
}

void MainMenuState::Unload()
{
}