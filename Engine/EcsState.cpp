#include "EcsState.h"
#include "Graphics.h"
#include <iostream>
#include <glm/glm.hpp>

// Define your components
struct Position
{
  float x, y;
};

struct Velocity
{
  float x, y;
};

struct DrawableRect
{
  float width, height;
  glm::vec4 color;
};

void EcsState::Load()
{
}

void EcsState::Init()
{
  std::cout << "Initializing EcsState...\n";

  // Create a system that updates position based on velocity
  ecs.system<Position, const Velocity>()
    .each([](Position& p, const Velocity& v) {
    p.x += v.x;
    p.y += v.y;
    std::cout << "Position: (" << p.x << ", " << p.y << ")\n";
      });

  // Create a drawing system
  ecs.system<const Position, const DrawableRect>()
    .each([](const Position& p, const DrawableRect& r)
      { Graphics::drawRect(p.x, p.y, r.width, r.height, r.color); });

  // Create an entity with Position, Velocity, and a DrawableRect
  auto e = ecs.entity()
    .insert([](Position& p, Velocity& v, DrawableRect& r)
      {
        p = { 10.0f, 20.0f };
        v = { 1.0f, 0.0f };
        r = { 50.0f, 50.0f, {1.0f, 0.0f, 0.0f, 1.0f} }; });
}

void EcsState::Update()
{
  Graphics::clearScreen(0.1f, 0.1f, 0.1f, 1.0f);
  ecs.progress();
}

void EcsState::Draw()
{

}

void EcsState::Free()
{
}

void EcsState::Unload()
{
}