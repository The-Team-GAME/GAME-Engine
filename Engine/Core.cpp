#include "Core.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "Graphics.h"
#include <glm/glm.hpp>
#include "StateManager.h"
#include "BasicState.h"
#include "MainMenuState.h"
#include "InputManager.h"
#include "ImGuiManager.h"
#include "StateChooser.inl"
#include "Time.h"
#include "PerformanceViewer.h"
#include <chrono>

namespace Engine
{

  Core* CORE;
  ECS::World* WORLD;
  ObjectFactory* FACTORY;
  Graphics* GRAPHICS;

  Core::Core()
  {
    isRunning = true;
    window = nullptr;
    CORE = this;
    WORLD = new ECS::World(); // TODO: check memory leak
    FACTORY = new ObjectFactory(WORLD);
    GRAPHICS = new Graphics();
  }

  Core::~Core()
  {
    // Nothing for now
  }

  int Core::init()
  {
    // Initialize accumulator
    Time::reset_accumulator();

    /* Initialize the library */
    if (!glfwInit())
      return -1;

    /* Create a windowed mode window and its OpenGL context */
    // TODO: window size should not be hardcoded
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

    glfwSwapInterval(0);

    if (glewInit() != GLEW_OK)
    {
      std::cout << "GLEW Initialization failed.\n";
    }

    std::cout << glGetString(GL_VERSION) << std::endl;

    StateManager::Init(BasicState::Get());

    ImGuiManager::init();

    Engine::GRAPHICS->init();

    return 0;
  }

  void Core::run()
  {
    while (!glfwWindowShouldClose(window) && isRunning)
    {
      // Updating of the accumulator (moved to Time)
      Time::update_time();
      Time::update_accumulator();

      // Only runs the main gameloop if the accumulator is more than the fixed dt
      if (!Time::should_update())
        continue;

      // Main gameloop here
      glfwPollEvents();
      ImGuiManager::begin();

      Input::Update();

      StateManager::GetCurrentState()->Update();
      StateManager::GetCurrentState()->Draw();

      WORLD->update(static_cast<float>(Time::get_fixed_dt()));

      StateChooserPanel();
      PerformanceViewerPanel();

      ImGuiManager::end();

      /* Swap front and back buffers */
      glfwSwapBuffers(window);

      // Reset accumulator (moved to Time)
      Time::subtract_fixed_dt();
    }
  }

  void Core::exit()
  {
    StateManager::Exit();
    Engine::GRAPHICS->exit();
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  GLFWwindow* Core::getWindow() const
  {
    return window;
  }

}