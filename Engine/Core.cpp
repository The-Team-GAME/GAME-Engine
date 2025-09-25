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
  s32 step_limit = 3;
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
      // START OF TIME UPDATES ==============================================
      // Updates the real_dt and increments the accumulator
      Time::update_time();
      accumulator += Time::get_real_dt();
      if (accumulator > Time::get_fixed_dt() * step_limit)
      { 
        // Caps the accumulator so each gameloop will only go through 5 loops max
        accumulator = Time::get_fixed_dt() * step_limit;
        std::cout << "nyo" << std::endl;
      }
      //std::cout << Time::calc_fps(Time::get_real_dt()) << std::endl;
      // END OF TIME UPDATES ================================================

      glfwPollEvents();
      IMGUI.begin();
      Input::Update();
      GRAPHICS.beginFrame();
      
      // Only runs the main gameloop if the accumulator is more than the fixed dt and limits the number of loops to loop_cap
      while(accumulator >= Time::get_fixed_dt())
      {        
        // To decrement the accumulator when each gameloop runs
        accumulator -= Time::get_fixed_dt();
        
        // Main gameloop here


        StateManager::GetCurrentState()->Update();

        WORLD.update();

        // Update audio system
        AUDIO.update();



        /* Swap front and back buffers */

        // End of gameloop

      }
        StateChooserPanel();
        PerformanceViewerPanel(Time::get_frame_cap());

      StateManager::GetCurrentState()->Draw();
      GRAPHICS.endFrame();
      IMGUI.end();
      glfwSwapBuffers(WINDOW.getWindow());

    }
  }
}