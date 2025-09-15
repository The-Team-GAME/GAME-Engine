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
  // TODO: window size should not be hardcoded
  int windowWidth = 800;
  int windowHeight = 600;

  // These are global instances of the core engine systems.
  // They are defined here and declared extern in Core.h for use throughout the engine.
  ECS::World WORLD{};
  ObjectFactory FACTORY{ &WORLD };
  WindowManager WINDOW{ windowWidth, windowHeight, "Monkey Stole My GPA" };
  Graphics GRAPHICS{ windowWidth, windowHeight };
  ImGuiManager IMGUI{};
  Time TIME{};

  Core::Core()
  {
    isRunning = true;
    StateManager::Init(BasicState::Get()); // TODO: to be removed
  }

  Core::~Core()
  {
    StateManager::Exit(); // TODO: to be removed 
  }

  void Core::run()
  {
    while (!glfwWindowShouldClose(WINDOW.getWindow()) && isRunning)
    {
      // Everything inside this loop ignores fixed timestep

      TIME.accumulate_time();

      while (TIME.should_update())
      {
        // Everything inside this loop is runned at a fixed timestep

        TIME.update_fixed_step();

        glfwPollEvents();

        IMGUI.begin();

        Input::Update();

        GRAPHICS.beginFrame();


        StateManager::GetCurrentState()->Update();
        StateManager::GetCurrentState()->Draw();

        WORLD.update();

        StateChooserPanel();
        PerformanceViewerPanel(TIME.target_fps);

        GRAPHICS.endFrame();

        IMGUI.end();

        /* Swap front and back buffers */
        glfwSwapBuffers(WINDOW.getWindow());
      }
    }
  }
}