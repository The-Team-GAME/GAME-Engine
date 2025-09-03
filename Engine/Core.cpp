#include "Core.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "Graphics.h"
#include <glm/glm.hpp>
#include "StateManager.h"
#include "BasicState.h"
#include "InputManager.h"

namespace Engine
{

  Core *CORE;

  Core::Core()
  {
    lastTime = 0;
    isRunning = true;
    window = nullptr;
    CORE = this; // Set the global pointer
  }

  Core::~Core()
  {
    // Nothing for now
  }

  int Core::init()
  {

    /* Initialize the library */
    if (!glfwInit())
      return -1;

    /* Create a windowed mode window and its OpenGL context */
    int windowWidth = 800;
    int windowHeight = 600;
    window = glfwCreateWindow(windowWidth, windowHeight, "Rectangle Drawer", NULL, NULL);
    if (!window)
    {
      std::cout << "Failed to create GLFW window" << std::endl;
      glfwTerminate();
      return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
      std::cout << "GLEW Initialization failed.\n";
    }

    std::cout << glGetString(GL_VERSION) << std::endl;

    StateManager::Init(BasicState::Get());

    Graphics::init();
  }

  void Core::run()
  {

    while (!glfwWindowShouldClose(window) && isRunning)
    {
      /* Render here */
      /*glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);*/

      Input::Update();

      StateManager::GetCurrentState()->Update();
      StateManager::GetCurrentState()->Draw();

      /* Swap front and back buffers */
      glfwSwapBuffers(window);

      /* Poll for and process events */
      glfwPollEvents();
    }
  }

  void Core::exit()
  {
    StateManager::Exit();
    Graphics::exit();
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  GLFWwindow *Core::getWindow() const
  {
    return window;
  }

}