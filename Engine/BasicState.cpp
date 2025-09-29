#include "BasicState.h"
#include "core.h"
#include "InputManager.h"
#include <glm/glm.hpp>
#include <iostream> 
#include <GLFW/glfw3.h>
#include <imgui.h>

void BasicState::Load()
{
  // Load fonts using ResourceManager
  Engine::RESOURCES.load_font("roboto", "Assets/fonts/Roboto-Regular.ttf", 48);
  fontId1 = Engine::RESOURCES.get_font("roboto");
  if (fontId1 == -1)
  {
    std::cout << "Failed to load roboto font for BasicState.\n";
  }

  Engine::RESOURCES.load_font("playfair", "Assets/fonts/PlayfairDisplay-VariableFont_wght.ttf", 48);
  fontId2 = Engine::RESOURCES.get_font("playfair");
  if (fontId2 == -1)
  {
    std::cout << "Failed to load playfair font for BasicState.\n";
  }

  // Load sound using ResourceManager
  Engine::RESOURCES.load_sound("move_sound", "Assets/sounds/move1.wav");
  moveSoundId = Engine::RESOURCES.get_sound("move_sound");
  if (moveSoundId == -1)
  {
    std::cout << "Failed to load move sound for BasicState.\n";
  }


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

  if (Engine::Input::IsKeyPressed(GLFW_KEY_P))
  {
    if (moveSoundId != -1)
    {
      Engine::AUDIO.playSound(moveSoundId);
      std::cout << "P key was pressed! Playing sound.\n";
    }
  }

  rotation += 45.0f * Engine::Time::get_fixed_dt();

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

  Engine::GRAPHICS.setCameraPosition(x, y);

  Engine::GRAPHICS.clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background


  Engine::GRAPHICS.drawRect({ 1.0f, 0.0f, 0.0f, 1.0f }, 0.0f, 0.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);        // Top left, Red
  Engine::GRAPHICS.drawRect({ 0.0f, 1.0f, 0.0f, 1.0f }, 1600.0f, 900.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);   // Bottom right, Green
  Engine::GRAPHICS.drawRect({ 0.0f, 0.0f, 1.0f, 1.0f }, 0.0f, 900.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);      // Bottom left, Blue
  Engine::GRAPHICS.drawRect({ 1.0f, 1.0f, 0.0f, 1.0f }, 1600.0f, 0.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);     // Top right, Yellow
  Engine::GRAPHICS.drawRect({ 1.0f, 0.0f, 1.0f, 1.0f }, 800.0f, 450.0f, 50.0f, 50.0f, rotation, Graphics::Origin::Center);    // Center, Magenta

  // Draw some text using the loaded font
  Engine::GRAPHICS.drawText(fontId1, "Hello", 250.0f, 250.0f, 1.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
  Engine::GRAPHICS.drawText(fontId2, "Hello", 250.0f, 300.0f, 1.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
  Engine::GRAPHICS.drawText(fontId1, "Press 'P' to play sound", 250.0f, 350.0f, 0.5f, { 1.0f, 1.0f, 0.0f, 1.0f });
}

void BasicState::Free()
{

}

void BasicState::Unload()
{
  // Unload resources using ResourceManager
  Engine::RESOURCES.unload_font("roboto");
  Engine::RESOURCES.unload_font("playfair");
  Engine::RESOURCES.unload_sound("move_sound");
}