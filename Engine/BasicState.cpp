#include "BasicState.h"
#include "Graphics.h"
#include <glm/glm.hpp>

void BasicState::Load()
{
}

void BasicState::Init()
{

}

void BasicState::Update()
{
}

void BasicState::Draw()
{
  Graphics::drawRect(100.0f, 100.0f, 200.0f, 50.0f, glm::vec3(1.0f, 0.0f, 0.0f));  // Red
  Graphics::drawRect(350.0f, 200.0f, 100.0f, 150.0f, glm::vec3(0.0f, 1.0f, 0.0f)); // Green
  Graphics::drawRect(50.0f, 400.0f, 75.0f, 75.0f, glm::vec3(0.0f, 0.0f, 1.0f));    // Blue

}

void BasicState::Free()
{
}

void BasicState::Unload()
{
}