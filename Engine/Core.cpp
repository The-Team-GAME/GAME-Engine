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
#include "WindowManager.h"

namespace Engine
{

  Core* CORE;
  ECS::World* WORLD;
  ObjectFactory* FACTORY;
  Graphics* GRAPHICS;
  ImGuiManager* IMGUI;
  WindowManager* WINDOW;

  Core::Core()
  {
    // TODO: window size should not be hardcoded
    int windowWidth = 800;
    int windowHeight = 600;

    isRunning = true;
    CORE = this;
    WORLD = new ECS::World();
    FACTORY = new ObjectFactory(WORLD);
    WINDOW = new WindowManager(windowWidth, windowHeight, "Monkey Stole My GPA");
    GRAPHICS = new Graphics(windowWidth, windowHeight);
    IMGUI = new ImGuiManager();


    StateManager::Init(BasicState::Get()); // TODO: to be removed
  }

  Core::~Core()
  {
    StateManager::Exit(); // TODO: to be removed

    delete IMGUI;
    delete GRAPHICS;
    delete WINDOW;
    delete FACTORY;
    delete WORLD;
  }

  void Core::run()
  {
    while (!glfwWindowShouldClose(WINDOW->getWindow()) && isRunning)
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
        glfwSwapBuffers(WINDOW->getWindow());
      }
    }
  }

  float Core::getDt() const
  {
    return static_cast<float>(time_manager.get_frame_dt());
  }
}