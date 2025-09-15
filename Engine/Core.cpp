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
  ImGuiManager* IMGUI;

  Core::Core()
  {
    // TODO: window size should not be hardcoded
    int windowWidth = 800;
    int windowHeight = 600;

    isRunning = true;
    window = nullptr;
    CORE = this;
    WORLD = new ECS::World();
    FACTORY = new ObjectFactory(WORLD);
    GRAPHICS = new Graphics(windowWidth, windowHeight);
    IMGUI = new ImGuiManager();

    /* Initialize the library */
    if (!glfwInit())
      return;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(windowWidth, windowHeight, "Monkey Stole My GPA", NULL, NULL);
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
  }

  Core::~Core()
  {
    StateManager::Exit();
    glfwDestroyWindow(window);
    glfwTerminate();
    Engine::FACTORY->unregisterAllObjects();

    delete IMGUI;
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

        IMGUI->begin();

        Input::Update();

        GRAPHICS->beginFrame();


        StateManager::GetCurrentState()->Update();
        StateManager::GetCurrentState()->Draw();

        WORLD->update();

        StateChooserPanel();
        PerformanceViewerPanel(time_manager.target_fps);

        GRAPHICS->endFrame();

        IMGUI->end();

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