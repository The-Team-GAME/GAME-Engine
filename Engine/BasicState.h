#pragma once

#include "StateManager.h"
#include "Graphics.h"
#include "InputManager.h"
#include <glm/glm.hpp>
#include <iostream>
#include <GLFW/glfw3.h>

class BasicState : public Engine::GameState<BasicState>
{
public:
  /*!
   * @brief Loads resources needed by the basic state.
   */
  void Load() override
  {
  }

  /*!
   * @brief Initializes the basic state.
   */
  void Init() override
  {
  }

  /*!
   * @brief Updates the basic state logic.
   */
  void Update() override
  {
    if (Engine::Input::IsKeyPressed(GLFW_KEY_SPACE))
    {
      std::cout << "Spacebar was pressed!\n";
    }

    if (Engine::Input::IsMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT))
    {
      glm::vec2 mousePos = Engine::Input::GetMousePosition();
      std::cout << "Left mouse button held at: " << mousePos.x << ", " << mousePos.y << "\n";
    }
  }

  /*!
   * @brief Renders the basic interface.
   */
  void Draw() override
  {
    Graphics::clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background

    Graphics::drawRect(100.0f, 100.0f, 200.0f, 50.0f, {1.0f, 0.0f, 0.0f, 1.0f});  // Red
    Graphics::drawRect(350.0f, 200.0f, 100.0f, 150.0f, {0.0f, 1.0f, 0.0f, 1.0f}); // Green
    Graphics::drawRect(50.0f, 400.0f, 75.0f, 75.0f, {0.0f, 0.0f, 1.0f, 1.0f});    // Blue
    Graphics::drawRect(70.0f, 400.0f, 75.0f, 75.0f, {1.0f, 1.0f, 1.0f, 0.2f});    // Transparent White
  }

  /*!
   * @brief Frees runtime resources used by the basic state.
   */
  void Free() override
  {
  }

  /*!
   * @brief Unloads resources used by the basic state.
   */
  void Unload() override
  {
  }
};