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

    /* Initialize the library */
    if (!glfwInit())
      return;

    /* Create a windowed mode window and its OpenGL context */
    // TODO: window size should not be hardcoded
    int windowWidth = 800;
    int windowHeight = 600;
    window = glfwCreateWindow(windowWidth, windowHeight, "Rectangle Drawer", NULL, NULL);
    if (!window)
    {
      std::cout << "Failed to create GLFW window" << std::endl;
      glfwTerminate();
      return;
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

    Engine::GRAPHICS->init(windowWidth, windowHeight);
  }

  Core::~Core()
  {
    StateManager::Exit();
    Engine::GRAPHICS->exit();
    glfwDestroyWindow(window);
    glfwTerminate();

    delete GRAPHICS;
    delete FACTORY;
    delete WORLD;
  }

  void Core::run()
  {
    while (!glfwWindowShouldClose(window) && isRunning)
    {
      // Everything inside this loop ignores fixed timestep

      time_manager.accumulate_time();

      while (time_manager.should_update())
      {
        // Everything inside this loop is runned at a fixed timestep

        time_manager.update_fixed_step();

        glfwPollEvents();
        ImGuiManager::begin();

        Input::Update();

        GRAPHICS->beginFrame();


        StateManager::GetCurrentState()->Update();
        StateManager::GetCurrentState()->Draw();

        WORLD->update();

        StateChooserPanel();
        PerformanceViewerPanel(time_manager.target_fps);

        GRAPHICS->endFrame();

        ImGuiManager::end();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);
      }
    }
  }

  float Core::getDt() const
  {
    return static_cast<float>(time_manager.get_frame_dt());
  }

  GLFWwindow* Core::getWindow() const
  {
    return window;
  }

}