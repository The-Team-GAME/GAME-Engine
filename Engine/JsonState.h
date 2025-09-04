#pragma once

#include "StateManager.h" 

class JsonState : public Engine::GameState<JsonState>
{
private:

public:
  void Load();
  void Init();
  void Update();
  void Draw();
  void Free();
  void Unload();
};