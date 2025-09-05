#pragma once

#include "StateManager.h"
#include "Graphics.h"

class SpriteState : public Engine::GameState<SpriteState>
{
private:
  int linearMonkeyImageId = -1;
  int nearestMonkeyImageId = -1;

public:
  void Load() {};

  void Init()
  {
    linearMonkeyImageId = Graphics::loadImage("Assets\\images\\monkey.png");
    nearestMonkeyImageId = Graphics::loadImage("Assets\\images\\monkey.png", Graphics::Filter::Nearest); // when loading pixel art, use nearest filter to avoid blurriness
  };

  void Update() {};

  void Draw()
  {
    Graphics::clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background

    Graphics::drawImage(linearMonkeyImageId, 200.0f, 150.0f, 15.0f * 10.0f, 21.0f * 10.0f);
    Graphics::drawImage(nearestMonkeyImageId, 400.0f, 150.0f, 15.0f * 10.0f, 21.0f * 10.0f);
  };

  void Free() {};

  void Unload() {};
};