#include "Core.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "FileHandler.h"
#include "Graphics.h"
#include <glm/glm.hpp>
#include "StateManager.h"
#include "BasicState.h"
#include "MainMenuState.h"
#include "ImGuiManager.h"
#include "StateChooser.inl"
#include "Time.h"
#include "PerformanceViewer.h"
#include "WindowManager.h"
#include "ResourceManagerPanel.h"
#include "EntityEditorPanel.h"
#include "CameraPanel.h"

namespace {
  f64 accumulator{};
  s32 step_limit = 3;
  bool physicsStepMode = false;  // Toggle for step-by-step physics
}
namespace Engine
{
  // TODO: window size should not be hardcoded
  int windowWidth = 1600;
  int windowHeight = 900;

  // These are global instances of the core engine systems.
  // They are defined here and declared extern in Core.h for use throughout the engine.
  ECS::World WORLD{};
  ObjectFactory FACTORY{ &WORLD };
  WindowManager WINDOW{ windowWidth, windowHeight, "Monkey Stole My GPA" };
  Graphics GRAPHICS{ windowWidth, windowHeight };
  ImGuiManager IMGUI{};
  AudioManager AUDIO{};
  ResourceManager RESOURCES{};

  Core::Core()
  {
    FileHandler::parse_file("Assets/objects/enemy.json");
    isRunning = true;

    // Register the graphics resize system
    WORLD.addEventListener<WindowResizeEvent>([](const WindowResizeEvent& event) {
      GRAPHICS.setViewport(event.width, event.height);
      }
    );

    StateManager::Init(BasicState::Get()); // TODO: to be removed
  }

  Core::~Core()
  {
    FileHandler::clear_json();
    StateManager::Exit(); // TODO: to be removed 

    WORLD.removeEventListener<WindowResizeEvent>();
  }

  void Core::run()
  {
    while (!glfwWindowShouldClose(WINDOW.getWindow()) && isRunning)
    {
      // START OF TIME UPDATES ==============================================
      // Updates the real_dt and increments the accumulator
      Time::update_time();

      if (!physicsStepMode)
      {
        // In normal mode, accumulate real delta time
        accumulator += Time::get_real_dt();
      }
      else {
        // In step mode, always allow at least one iteration to check input
        accumulator = Time::get_fixed_dt();
      }

      if (accumulator > Time::get_fixed_dt() * step_limit)
      {
        // Caps the accumulator so each gameloop will only go through 5 loops max
        accumulator = Time::get_fixed_dt() * step_limit;
        //std::cout << "nyo" << std::endl; // Should probably warn the user if this happens
      }
      //std::cout << "DT: " << Time::get_real_dt() << " FPS: " << Time::calc_fps(Time::get_real_dt()) << std::endl;
      // END OF TIME UPDATES ================================================

      glfwPollEvents();
      AUDIO.update();
      IMGUI.begin();
      GRAPHICS.beginFrame();


      // Only runs the main gameloop if the accumulator is more than the fixed dt and limits the number of loops to loop_cap
      while (accumulator >= Time::get_fixed_dt())
      {
        // To decrement the accumulator when each gameloop runs
        accumulator -= Time::get_fixed_dt();

        // Update input right before the main gameloop. This needs to be here because if it is out side the while loop,
        // WORLD.update() won't have a chance to react before the input state is cleared on the next frame. 
        Input::Update();

        if (Engine::Input::IsKeyPressed(GLFW_KEY_ESCAPE))
        {
          isRunning = false;
        }

        // Demonstrate error catching is working
        if (Engine::Input::IsKeyPressed(GLFW_KEY_F11))
        {
          throw std::runtime_error("F11 was pressed");
        }

        if (Engine::Input::IsKeyPressed(GLFW_KEY_P))
        {
          physicsStepMode = !physicsStepMode;
          std::cout << "Physics Step Mode: " << (physicsStepMode ? "ON" : "OFF") << std::endl;
        }

        bool shouldRunPhysics = !physicsStepMode || Engine::Input::IsKeyPressed(GLFW_KEY_RIGHT_BRACKET);

        if (shouldRunPhysics)
        {
          // Main gameloop here
          StateManager::GetCurrentState()->Update();
          WORLD.update();
        }

        IMGUI.update();

        // End of gameloop
      }

      // Rendering Start ========================================
      StateChooserPanel();
      PerformanceViewerPanel();
      ResourceManagerPanel();
      EntityEditorPanel();
      CameraPanel();
      StateManager::GetCurrentState()->Draw();
      WORLD.draw();
      GRAPHICS.endFrame();
      IMGUI.end();
      glfwSwapBuffers(WINDOW.getWindow());
      // Rendering End ==========================================
    }
  }
}