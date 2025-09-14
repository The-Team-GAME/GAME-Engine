#include "BasicState.h"
#include "core.h"
#include "InputManager.h"
#include <glm/glm.hpp>
#include <iostream> 
#include <GLFW/glfw3.h>
#include <imgui.h>

void BasicState::Load()
{
}

void BasicState::Init()
{
}

void BasicState::Update()
{
  if (Engine::Input::IsKeyPressed(GLFW_KEY_SPACE))
  {
    std::cout << "Spacebar was pressed!\n";
  }

  /*if (Engine::Input::IsMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT))
  {
    glm::vec2 mousePos = Engine::Input::GetMousePosition();
    std::cout << "Left mouse button held at: " << mousePos.x << ", " << mousePos.y << "\n";
  }*/
}

void BasicState::Draw()
{
  static float x = 0.0f;
  static float y = 0.0f;

  ImGui::Begin("Camera");
  if (ImGui::Button("Reset")) {
    x = 0.0f;
    y = 0.0f;
  }
  ImGui::SliderFloat("X", &x, -800.0f, 800.0f);
  //Reset button
  ImGui::SliderFloat("Y", &y, -800.0f, 800.0f);
  ImGui::End();

  Engine::GRAPHICS->setCameraPosition(x, y);

  Engine::GRAPHICS->clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background

  static float rotation = 0.0f;
  rotation += 45.0f * static_cast<float>(Engine::CORE->getDt());

  Engine::GRAPHICS->drawRect({ 1.0f, 0.0f, 0.0f, 1.0f }, 0.0f, 0.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);       // Top left, Red
  Engine::GRAPHICS->drawRect({ 0.0f, 1.0f, 0.0f, 1.0f }, 800.0f, 600.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);   // Bottom right, Green
  Engine::GRAPHICS->drawRect({ 0.0f, 0.0f, 1.0f, 1.0f }, 0.0f, 600.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);     // Bottom left, Blue
  Engine::GRAPHICS->drawRect({ 1.0f, 1.0f, 0.0f, 1.0f }, 800.0f, 0.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);     // Top right, Yellow
  Engine::GRAPHICS->drawRect({ 1.0f, 0.0f, 1.0f, 1.0f }, 400.0f, 300.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);// Center, Magenta
}

void BasicState::Free()
{
}

void BasicState::Unload()
{
}