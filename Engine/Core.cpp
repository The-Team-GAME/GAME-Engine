#include "Core.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "Graphics.h"
#include <glm/glm.hpp>

namespace Engine
{

  Core *CORE;

  Core::Core()
  {
    lastTime = 0;
    isRunning = true;
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

    Graphics::init();
  }

  void Core::gameLoop()
  {
    while (!glfwWindowShouldClose(window) && isRunning)
    {
      /* Render here */
      glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      // Draw rectangles of different sizes and colors
      Graphics::drawRect(100.0f, 100.0f, 200.0f, 50.0f, glm::vec3(1.0f, 0.0f, 0.0f));  // Red
      Graphics::drawRect(350.0f, 200.0f, 100.0f, 150.0f, glm::vec3(0.0f, 1.0f, 0.0f)); // Green
      Graphics::drawRect(50.0f, 400.0f, 75.0f, 75.0f, glm::vec3(0.0f, 0.0f, 1.0f));    // Blue

      /* Swap front and back buffers */
      glfwSwapBuffers(window);

      /* Poll for and process events */
      glfwPollEvents();
    }
  }

  void Core::exit()
  {
    Graphics::exit();
    glfwDestroyWindow(window);
    glfwTerminate();
  }

}