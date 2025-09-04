#pragma once

#include "StateManager.h"
#include "ECS.h"

class EcsState : public Engine::GameState<EcsState>
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