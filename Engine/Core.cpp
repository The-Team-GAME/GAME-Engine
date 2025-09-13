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

namespace {
  // just local variables
  f64 accumulator;
  s32 loop_cap{ 50 };
}

namespace Engine
{

  Core* CORE;
  ECS::World* WORLD;
  ObjectFactory* FACTORY;
  Graphics* GRAPHICS;

  Core::Core()
  {
    dt = 0.0f;
    isRunning = true;
    window = nullptr;
    dt = 0.0f;
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
    // For game loop
    accumulator = 0;


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
      //Updating of the accumulator
      Time::update_time();
      accumulator += Time::get_gameloop_dt();

      std::cout << accumulator << std::endl;

      // Only runs the main gameloop if the accumulator is more than the fixed dt and limits the number of loops to loop_cap
      if (accumulator < Time::get_dt())
        continue;

      // Main gameloop here
      glfwPollEvents();
      ImGuiManager::begin();

      Input::Update();

      StateManager::GetCurrentState()->Update();
      StateManager::GetCurrentState()->Draw();

      WORLD->update(static_cast<float>(Time::get_dt()));

      StateChooserPanel();
      PerformanceViewerPanel();

      ImGuiManager::end();

      /* Swap front and back buffers */
      glfwSwapBuffers(window);

      // End of gameloop
      // Reset accumulator
      accumulator = 0.0;
    }
  }

  void Core::exit()
  {
    StateManager::Exit();
    Engine::GRAPHICS->exit();
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  float Core::getDt() const // TODO remove eventually?
  {
    return Time::get_dt();
  }

  GLFWwindow* Core::getWindow() const
  {
    return window;
  }

}