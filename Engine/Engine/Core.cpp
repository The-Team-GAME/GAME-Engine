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
#include "Precompiled.h"
#include <iostream>
#include "FileHandler.h"
#include "SceneManager.h"
#include "ImGuiManager.h"
#include "Time.h"
#include "GUI/PerformanceViewer.h"
#include "WindowManager.h"
#include "GUI/ResourceManagerPanel.h"
#include "GUI/EntityEditorPanel.h"
#include "GUI/CameraPanel.h"
#include "Perf.h"
#include "InputManager.h"
#include "Audio/AudioPlayer.h"
#include "Audio/AudioPlayerInternal.h"
#include "Resources.h"
#include "ECS/ECS.h"
#include "Events/WindowEvents.h"
#include "Systems/Systems.h"

namespace {
  f64 accumulator{};
  s32 step_limit = 3;
  bool do_render = false;
  bool physics_step_mode = false;  // Toggle for step-by-step physics
}

namespace Engine
{
  /*=================================================================================
  Function Brief:
  Constructor that initializes all engine subsystems including window, graphics,
  ImGui, audio, and the ECS World:: Loads configuration and sets up event listeners.
  ==================================================================================*/
  Core::Core(s32 window_width, s32 window_height, const char* window_title, std::string first_scene)
  {
    isRunning = true;

    WindowManager::init(window_width, window_height, window_title);

    ImGuiManager::init();

    // Initialize graphics renderer
    if (!Engine::Graphics::init()) {
      throw std::runtime_error("Failed to initialize graphics renderer");
    }

    // Initialize resource manager
    if (!Engine::Resources::init()) {
      throw std::runtime_error("Failed to initialize resource manager");
    }

    // Initialize audio manager
    if (!Engine::Audio::init()) {
      throw std::runtime_error("Failed to initialize audio manager");
    }

    Engine::Graphics::set_viewport({ 0, 0, window_width, window_height });

    // Register the graphics resize system 
    Engine::ECS::add_event_listener<Engine::Events::WindowResizeEvent>([](const Engine::Events::WindowResizeEvent& event)
      {
        Engine::Graphics::set_viewport({ 0, 0, event.width, event.height });
      }
    );

		SceneManager::load_scene(first_scene);
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
    // CORE INITIALIZATION
    register_engine_systems();

    while (!glfwWindowShouldClose(WindowManager::get_window()) && isRunning)
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

      Engine::Audio::update();
      ImGuiManager::begin();

      // Only runs the main gameloop if the accumulator is more than the fixed dt and limits the number of loops to loop_cap
      while (accumulator >= Time::get_fixed_dt())
      {
        // To decrement the accumulator when each gameloop runs
        accumulator -= Time::get_fixed_dt();

        // Update input right before the main gameloop. This needs to be here because if it is out side the while loop,
        // World::update() won't have a chance to react before the input state is cleared on the next frame. 
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
          Engine::SceneManager::update();
          Engine::ECS::update();
        }

        ImGuiManager::update();

        // End of gameloop
      }

      // Rendering Start ========================================
      PerformanceViewerPanel();
      ResourceManagerPanel();
      EntityEditorPanel();
      CameraPanel();

      // Clear screen (automatically begins batch)
      // start of draw stuff
      Engine::SceneManager::draw();
      Engine::ECS::draw();

      // end of draw stuff (flush remaining batches)
      Engine::Graphics::end_batch();

      ImGuiManager::end();
      glfwSwapBuffers(WindowManager::get_window());
      // Rendering End ==========================================
    }

    // CORE CLEANUP
    unregister_engine_systems();
  }

  /*=================================================================================
  Function Brief:
  Destructor that cleans up all engine resources, shuts down subsystems, and
  removes event listeners. Ensures proper cleanup before application exit.
  ==================================================================================*/
  Core::~Core()
  {
    Engine::SceneManager::cleanup();
    //Engine::ECS::remove_event_listener<WindowResizeEvent>();
    Engine::Audio::exit();
    Engine::Resources::exit();
    Engine::Graphics::exit();
    ImGuiManager::exit();
    WindowManager::exit();
  }

  void Core::stop()
  {
    isRunning = false;
  }
}
