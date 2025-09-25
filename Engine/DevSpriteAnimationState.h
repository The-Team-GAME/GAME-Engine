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
  int marioSpriteSheetId = -1;
  float animationTime = 0.0f;

public:
  void Load() {};

  void Init() {
    marioSpriteSheetId = Engine::GRAPHICS.loadSpriteSheet(
      "Assets/images/mario.png",
      96 / 3, // frame width
      32, // frame height
      3, // total frames
      8.0f, // frames per second
      Graphics::MinifyingFilter::Nearest
    );
  };

  void Update() {
    animationTime += 0.016f;
  };

  void Draw() {
    Engine::GRAPHICS.clearScreen(0.1f, 0.1f, 0.1f, 1.0f);

    // Draw animated sprite
    Engine::GRAPHICS.drawAnimatedSprite(marioSpriteSheetId, 200.0f, 150.0f, 96.0f, 96.0f, animationTime);
  };

  void Free() {};

  void Unload() {};

};