#pragma once

#include "StateManager.h"
#include "Graphics.h"
#include "ObjectFactory.h"

class GameObjectState : public Engine::GameState<GameObjectState>
{
private:

public:
  void Load() {};

  void Init()
  {
    Engine::FACTORY->fromJsonFile("Assets\\objects\\enemy.json");
  };

  void Update() {};

  void Draw()
  {
    Graphics::clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
  };

  void Free() {};

  void Unload() {};
};