#pragma once

#include "StateManager.h"
#include "Graphics.h"
#include "InputManager.h"
#include <glm/glm.hpp>
#include <iostream>
#include <GLFW/glfw3.h>

class MainMenuState : public Engine::GameState<MainMenuState>
{
public:
  /*!
   * @brief Loads resources needed by the main menu state.
   */
  void Load() override
  {
  }

  /*!
   * @brief Initializes the main menu state.
   */
  void Init() override
  {
  }

  /*!
   * @brief Updates the main menu state logic.
   */
  void Update() override
  {
  }

  /*!
   * @brief Renders the main menu interface.
   */
  void Draw() override
  {
    Graphics::clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
  }

  /*!
   * @brief Frees runtime resources used by the main menu state.
   */
  void Free() override
  {
  }

  /*!
   * @brief Unloads resources used by the main menu state.
   */
  void Unload() override
  {
  }
};