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

namespace Engine
{

  Core* CORE;
  ECS::World* WORLD;
  ObjectFactory* FACTORY;

  Core::Core()
  {
    lastTime = 0.0f;
    dt = 0.0f;
    isRunning = true;
    window = nullptr;
    dt = 0.0f;
    CORE = this; // Set the global pointer
    WORLD = new ECS::World();
    FACTORY = new ObjectFactory(WORLD);
  }

  Core::~Core()
  {
    // Nothing for now
  }

  int Core::init()
  {

    /* Initialize the library */
    if (!glfwInit())
      return -1;

    /* Create a windowed mode window and its OpenGL context */
    // TODO: window size should not be hardcoded
    int windowWidth = 800;
    int windowHeight = 600;
    window = glfwCreateWindow(windowWidth, windowHeight, "Rectangle Drawer", NULL, NULL);
    if (!window)
    {
      std::cout << "Failed to create GLFW window" << std::endl;
      glfwTerminate();
      return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
      std::cout << "GLEW Initialization failed.\n";
    }

    std::cout << glGetString(GL_VERSION) << std::endl;

    StateManager::Init(BasicState::Get());

    ImGuiManager::init();

    Graphics::init();

    return 0;
  }

  void Core::run()
  {
    lastTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window) && isRunning)
    {
      float currenttime = static_cast<float>(glfwGetTime()) * 1000.0f;
      dt = (currenttime - lastTime) / 1000.0f;
      lastTime = currenttime;

      glfwPollEvents();
      ImGuiManager::begin();

      Input::Update();

      StateManager::GetCurrentState()->Update();
      StateManager::GetCurrentState()->Draw();

      WORLD->update(dt);

      StateChooserPanel();

      ImGuiManager::end();

      /* Swap front and back buffers */
      glfwSwapBuffers(window);
    }
  }

  void Core::exit()
  {
    StateManager::Exit();
    Graphics::exit();
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  GLFWwindow* Core::getWindow() const
  {
    return window;
  }

}