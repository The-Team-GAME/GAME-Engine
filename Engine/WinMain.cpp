#include <iostream> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Graphics.h"
#include "Core.h"

int main(void)
{
  Engine::Core* engine = new Engine::Core();

  engine->init();
  engine->gameLoop();
  engine->exit();

  return 0;
}