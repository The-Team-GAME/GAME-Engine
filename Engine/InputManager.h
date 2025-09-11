#pragma once
#include <glm/glm.hpp>

namespace Engine
{
  /**
   * @class Input
   * @brief A static class for handling keyboard and mouse input.
   *
   * This class provides methods to check the state of keyboard keys and mouse buttons.
   * The Update() method must be called once per frame before any other methods are used.
   */
  class Input
  {
  public:
    /**
     * @brief Updates the internal state of the input manager.
     * This should be called once at the beginning of each frame.
     */
    static void Update();

    // Keyboard input
    /**
     * @brief Checks if a key was just pressed in the current frame.
     * @param key The key code to check (e.g., from GLFW).
     * @return true if the key was pressed in this frame, false otherwise.
     */
    static bool IsKeyPressed(int key);

    /**
     * @brief Checks if a key was just released in the current frame.
     * @param key The key code to check.
     * @return true if the key was released in this frame, false otherwise.
     */
    static bool IsKeyReleased(int key);

    /**
     * @brief Checks if a key is currently being held down.
     * @param key The key code to check.
     * @return true if the key is held down, false otherwise.
     */
    static bool IsKeyHeld(int key);

    // Mouse input
    /**
     * @brief Checks if a mouse button was just pressed in the current frame.
     * @param button The mouse button code to check (e.g., from GLFW).
     * @return true if the mouse button was pressed in this frame, false otherwise.
     */
    static bool IsMouseButtonPressed(int button);

    /**
     * @brief Checks if a mouse button was just released in the current frame.
     * @param button The mouse button code to check.
     * @return true if the mouse button was released in this frame, false otherwise.
     */
    static bool IsMouseButtonReleased(int button);

    /**
     * @brief Checks if a mouse button is currently being held down.
     * @param button The mouse button code to check.
     * @return true if the mouse button is held down, false otherwise.
     */
    static bool IsMouseButtonHeld(int button);

    /**
     * @brief Gets the current position of the mouse cursor in window coordinates.
     * @return glm::vec2 A vector containing the x and y position of the mouse.
     */
    static glm::vec2 GetMousePosition();
  };
}