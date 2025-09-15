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

  Core* CORE;
  ECS::World WORLD{};
  ObjectFactory FACTORY{ &WORLD };
  WindowManager WINDOW{ windowWidth, windowHeight, "Monkey Stole My GPA" };
  Graphics GRAPHICS{ windowWidth, windowHeight };
  ImGuiManager IMGUI{};

  Core::Core()
  {
    

    isRunning = true;

    CORE = this;

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

      time_manager.accumulate_time();

      while (time_manager.should_update())
      {
        // Everything inside this loop is runned at a fixed timestep

        time_manager.update_fixed_step();

        glfwPollEvents();

        IMGUI.begin();

        Input::Update();

        GRAPHICS.beginFrame();


        StateManager::GetCurrentState()->Update();
        StateManager::GetCurrentState()->Draw();

        WORLD.update();

        StateChooserPanel();
        PerformanceViewerPanel(time_manager.target_fps);

        GRAPHICS.endFrame();

        IMGUI.end();

        /* Swap front and back buffers */
        glfwSwapBuffers(WINDOW.getWindow());
      }
    }
  }

  float Core::getDt() const
  {
    return static_cast<float>(time_manager.get_frame_dt());
  }
}