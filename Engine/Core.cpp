//-----------------------------------------------------------------------------------
// file:    Core.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (50%)
//          Cheng Ming Xian (50%)

// email:   chew.s@digipen.edu, c.mingxian@digipen.edu
//
// brief:   This file implements the Core class which manages the main game loop.
//          Handles fixed timestep updates, rendering, input processing, and
//          coordinates all major engine subsystems.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Core.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "FileHandler.h"
#include "Graphics.h"
#include "StateManager.h"
//#include "BasicState.h"
#include "ImGuiManager.h"
#include "StateChooser.h"
#include "Time.h"
#include "PerformanceViewer.h"
#include "WindowManager.h"
//#include "ResourceManagerPanel.h"
#include "EntityEditorPanel.h"
//#include "CameraPanel.h"
#include "Config.h"
#include "Perf.h"
#include "InputManager.h"
#include "BasicEvents.h"
//#include "ShowcaseState.h"

namespace {
  f64 accumulator{};
  s32 step_limit = 3;
  bool do_render = false;
  bool physics_step_mode = false;  // Toggle for step-by-step physics
}

namespace Engine
{

  /* ========================================================================= //
  // Global engine system instances                                            //
  // These are defined here and declared extern in Core.h for global access   //
  // ========================================================================= */
  // They are defined here and declared extern in Core.h for use throughout the engine.
  ECS::World WORLD{};
  //ObjectFactory FACTORY{ &WORLD };

  WindowManager WINDOW;

  //Graphics GRAPHICS;

  //LevelLoader LEVEL_LOADER{ &WORLD, &FACTORY };

  AudioManager AUDIO{};

  //ResourceManager RESOURCES{};

  Perf PERFORMANCE{};

  /*=================================================================================
  Function Brief:
  Constructor that initializes all engine subsystems including window, graphics,
  ImGui, audio, and the ECS world. Loads configuration and sets up event listeners.
  ==================================================================================*/
  Core::Core()
  {
    Config::load_config();
    s32 win_width = Config::get_win_width();
    s32 win_height = Config::get_win_height();

    WINDOW.set_params(win_width, win_height, "Monkey Stole My GPA");
    WINDOW.window_init();

    ImguiMgr::imgui_init();

    // Initialize graphics renderer
    if (!Engine::Graphics::init()) {
      throw std::runtime_error("Failed to initialize graphics renderer");
    }

    // Initialize resource manager
    if (!Engine::Resources::init()) {
      throw std::runtime_error("Failed to initialize resource manager");
    }

    /*GRAPHICS.set_params(win_width, win_height);
    GRAPHICS.init_graphics();*/
    isRunning = true;

    // Register the graphics resize system
    WORLD.add_event_listener<WindowResizeEvent>([](const WindowResizeEvent& event) {
      //GRAPHICS.set_viewport(event.width, event.height); //TODO: bring back when implmemented in new graphics
      }
    );

    StateManager::init(DevSpriteAnimationState::get()); // TODO: to be removed
  }

  /*=================================================================================
  Function Brief:
  Main game loop that runs continuously until the window is closed. Implements
  fixed timestep updates with accumulator for consistent physics, handles input,
  updates all systems, and renders the scene. Supports physics step mode for debugging.

  Where to call it:
  Called once after Core construction to start the game loop.
  ==================================================================================*/
  void Core::run()
  {
    while (!glfwWindowShouldClose(WINDOW.get_window()) && isRunning)
    {
      // START OF TIME UPDATES ==============================================
      // Updates the real_dt and increments the accumulator
      Time::update_time();

      if (!physics_step_mode)
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
        // Caps the accumulator so each gameloop will only go through 3 loops max
        accumulator = Time::get_fixed_dt() * step_limit;
      }
      //std::cout << "DT: " << Time::get_real_dt() << " FPS: " << Time::calc_fps(Time::get_real_dt()) << std::endl;
      // END OF TIME UPDATES ================================================

      glfwPollEvents();
      AUDIO.update();
      ImguiMgr::begin();
      //GRAPHICS.begin_frame();

      // Only runs the main gameloop if the accumulator is more than the fixed dt and limits the number of loops to loop_cap
      while (accumulator >= Time::get_fixed_dt())
      {
        // To decrement the accumulator when each gameloop runs
        accumulator -= Time::get_fixed_dt();

        // Update input right before the main gameloop. This needs to be here because if it is out side the while loop,
        // WORLD.update() won't have a chance to react before the input state is cleared on the next frame. 
        Input::update();

        if (Engine::Input::is_key_pressed(GLFW_KEY_ESCAPE))
        {
          isRunning = false;
        }

        // Demonstrate error catching is working
        if (Engine::Input::is_key_pressed(GLFW_KEY_F11))
        {
          throw std::runtime_error("F11 was pressed");
        }

        if (Engine::Input::is_key_pressed(GLFW_KEY_P))
        {
          physics_step_mode = !physics_step_mode;
          std::cout << "Physics Step Mode: " << (physics_step_mode ? "ON" : "OFF") << std::endl;
        }

        bool shouldRunPhysics = !physics_step_mode || Engine::Input::is_key_pressed(GLFW_KEY_RIGHT_BRACKET);

        if (shouldRunPhysics)
        {
          // Main gameloop here
          StateManager::get_current_state()->update();
          WORLD.update();
        }

        ImguiMgr::update();

        // End of gameloop
      }

      // Rendering Start ========================================
      StateChooserPanel();
      PerformanceViewerPanel();
      //ResourceManagerPanel();
      EntityEditorPanel();
      //CameraPanel();
      // start of draw stuff
      WORLD.draw();
      StateManager::get_current_state()->draw();
      //GRAPHICS.end_frame();

      // end of draw stuff
      ImguiMgr::end();
      glfwSwapBuffers(WINDOW.get_window());
      // Rendering End ==========================================
    }
  }

  /*=================================================================================
  Function Brief:
  Destructor that cleans up all engine resources, shuts down subsystems, and
  removes event listeners. Ensures proper cleanup before application exit.
  ==================================================================================*/
  Core::~Core()
  {
    FileHandler::clear_json();

    ImguiMgr::imgui_exit();

    StateManager::exit(); // TODO: to be removed 

    WORLD.remove_event_listener<WindowResizeEvent>();

    // Cleanup graphics resources
    Engine::Resources::exit();
    Engine::Graphics::exit();
  }

}