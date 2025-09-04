#include "BasicState.h"
#include "Graphics.h"
#include "InputManager.h"
#include <glm/glm.hpp>
#include <iostream> 
#include <GLFW/glfw3.h>

#include "ImGuiManager.h"
#include "RectEditor.h"

void BasicState::Load()
{
}

void BasicState::Init()
{
    Engine::ImGuiManager::addPanel(Engine::RectEditor::DrawPanel);
}

void BasicState::Update()
{
  if (Engine::Input::IsKeyPressed(GLFW_KEY_SPACE))
  {
    std::cout << "Spacebar was pressed!\n";
  }

  if (Engine::Input::IsMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT))
  {
    glm::vec2 mousePos = Engine::Input::GetMousePosition();
    std::cout << "Left mouse button held at: " << mousePos.x << ", " << mousePos.y << "\n";
  }

  Engine::RectEditor::Update();
}

void BasicState::Draw()
{
  Graphics::clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background

  Graphics::drawRect(100.0f, 100.0f, 200.0f, 50.0f, { 1.0f, 0.0f, 0.0f, 1.0f });  // Red
  Graphics::drawRect(350.0f, 200.0f, 100.0f, 150.0f, { 0.0f, 1.0f, 0.0f, 1.0f }); // Green
  Graphics::drawRect(50.0f, 400.0f, 75.0f, 75.0f, { 0.0f, 0.0f, 1.0f, 1.0f });    // Blue
  Graphics::drawRect(70.0f, 400.0f, 75.0f, 75.0f, { 1.0f, 1.0f, 1.0f, 0.2f });    // Transparent White

  for (const auto& r : Engine::RectEditor::GetRects())
  {
      Graphics::drawRect(r.x, r.y, r.w, r.h, r.color);
  }

  Engine::Rect preview;
  if (Engine::RectEditor::GetPreviewRect(preview))
  {
      Graphics::drawRect(preview.x, preview.y, preview.w, preview.h, preview.color);
  }

}

void BasicState::Free()
{
}

void BasicState::Unload()
{
}