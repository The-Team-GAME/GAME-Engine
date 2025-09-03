#include <iostream> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Graphics.h"
#include "Core.h"
#include "StateManager.h"
#include "BasicState.h"

int main(void)
{
  Engine::Core* engine = new Engine::Core();


  Engine::StateManager::PushState(BasicState::Get());

  engine->init();
  engine->run();
  engine->exit();

  return 0;
}