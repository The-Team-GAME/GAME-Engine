#pragma once

#include "StateManager.h"
#include "core.h"
#include "ObjectFactory.h"
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <stack>
#include <iostream> // For debugging output





class DevSpriteAnimationState : public Engine::GameState<DevSpriteAnimationState> {

private:
  int marioImageId = -1;

public:
  void Load() {};

  void Init() {
    marioImageId = Engine::GRAPHICS.loadTexture("Assets/images/mario.png", Graphics::MinifyingFilter::Nearest);
  };



  void Update() {};



  void Draw() {

    //image stuff

    Engine::GRAPHICS.clearScreen(0.1f, 0.1f, 0.1f, 1.0f);

    Engine::GRAPHICS.drawTexture(marioImageId, 200.0f, 150.0f, 96.0f, 32.0f);



  };



  void Free() {};

  void Unload() {};

};