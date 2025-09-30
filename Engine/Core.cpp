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
  bool physicsStepMode = false;
}

namespace Engine
{
  int windowWidth = 1600;
  int windowHeight = 900;

  ECS::World WORLD{};
  ObjectFactory FACTORY{ &WORLD };
  LevelLoader LEVEL_LOADER{ &WORLD, &FACTORY };
  WindowManager WINDOW{ windowWidth, windowHeight, "Monkey Stole My GPA" };
  Graphics GRAPHICS{ windowWidth, windowHeight };
  ImGuiManager IMGUI{};
  AudioManager AUDIO{};
  ResourceManager RESOURCES{};

  Core::Core()
  {
    FileHandler::parse_file("Assets/objects/enemy.json");
    isRunning = true;

    WORLD.addEventListener<WindowResizeEvent>([](const WindowResizeEvent& event) {
      GRAPHICS.setViewport(event.width, event.height);
      }
    );

    StateManager::Init(BasicState::Get());
  }

  Core::~Core()
  {
    FileHandler::clear_json();
    StateManager::Exit();

    WORLD.removeEventListener<WindowResizeEvent>();
  }

  void Core::run()
  {
    while (!glfwWindowShouldClose(WINDOW.getWindow()) && isRunning)
    {
      Time::update_time();

      if (!physicsStepMode)
      {
        accumulator += Time::get_real_dt();
      }
      else {
        accumulator = Time::get_fixed_dt();
      }

      if (accumulator > Time::get_fixed_dt() * step_limit)
      {
        accumulator = Time::get_fixed_dt() * step_limit;
      }

      glfwPollEvents();
      AUDIO.update();
      IMGUI.begin();
      GRAPHICS.beginFrame();

      while (accumulator >= Time::get_fixed_dt())
      {
        accumulator -= Time::get_fixed_dt();

        Input::Update();

        if (Engine::Input::IsKeyPressed(GLFW_KEY_ESCAPE))
        {
          isRunning = false;
        }

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
          StateManager::GetCurrentState()->Update();
          WORLD.update();
        }

        IMGUI.update();
      }

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
    }
  }
}