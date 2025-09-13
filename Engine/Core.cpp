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

    // static std::chrono::milliseconds curr_time = std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::steady_clock::now()).time_since_epoch());

    // static float curr_time = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window) && isRunning)
    {
      //Updating of the accumulator
      Time::update_time();
      accumulator += Time::get_internal_dt();

      // Only runs the main gameloop if the accumulator is more than the fixed dt and limits the number of loops to loop_cap
      if (accumulator < Time::get_fixed_dt())
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

      // Reset accumulator
      // (The reason wqe don't just set it to zero is because if the frame took longer than the fixed dt, we want to keep the extra time for the next frame)
      accumulator -= Time::get_fixed_dt();

      /*std::chrono::milliseconds new_time = std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::steady_clock::now()).time_since_epoch());
      dt = static_cast<f64>((new_time - curr_time).count()) / 1000.0;
      curr_time = new_time;*/

      /*float new_time = static_cast<float>(glfwGetTime()) * 1000.0f;
      dt = (new_time - curr_time) / 1000.0f;
      curr_time = new_time;*/
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
    //return Time::get_fixed_dt();
    return dt;
  }

  GLFWwindow* Core::getWindow() const
  {
    return window;
  }

}