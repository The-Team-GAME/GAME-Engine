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

namespace {
  f64 accumulator{};
}
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
  AudioManager AUDIO{};

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
      //Updating of the real_dt and accumulator
      Time::update_time();

      if (accumulator < Time::get_fixed_dt() * 5)
      { // Caps the accumulator so each gameloop will only go through 5 loops max
        accumulator += Time::get_real_dt();
      }
      

      glfwPollEvents();
      // Only runs the main gameloop if the accumulator is more than the fixed dt and limits the number of loops to loop_cap
      while(accumulator >= Time::get_fixed_dt())
      {
        // Main gameloop here
        
        IMGUI.begin();
        Input::Update();
        GRAPHICS.beginFrame();

        StateManager::GetCurrentState()->Update();
        StateManager::GetCurrentState()->Draw();

        WORLD.update();

        // Update audio system
        AUDIO.update();

        StateChooserPanel();
        PerformanceViewerPanel(Time::get_frame_cap());

        GRAPHICS.endFrame();

        IMGUI.end();

        /* Swap front and back buffers */

        // End of gameloop
        // To decrement the accumulator when each gameloop runs
        accumulator -= Time::get_fixed_dt();
      }
      glfwSwapBuffers(WINDOW.getWindow());

    }
  }
}