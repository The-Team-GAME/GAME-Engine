#pragma once

#include "StateManager.h"
#include "core.h"

class SpriteState : public Engine::GameState<SpriteState>
{
private:
  int linearMonkeyImageId = -1;
  int nearestMonkeyImageId = -1;

public:
  void Load() {};

  void Init()
  {
    linearMonkeyImageId =  Engine::GRAPHICS->loadTexture("Assets/images/monkey.png");
    nearestMonkeyImageId =  Engine::GRAPHICS->loadTexture("Assets/images/monkey.png",  Graphics::MinifyingFilter::Nearest); // when loading pixel art, use nearest filter to avoid blurriness
  };

  void Update() {};

  void Draw()
  {
     Engine::GRAPHICS->clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background

     Engine::GRAPHICS->drawTexture(linearMonkeyImageId, 200.0f, 150.0f, 15.0f * 10.0f, 21.0f * 10.0f);
     Engine::GRAPHICS->drawTexture(nearestMonkeyImageId, 400.0f, 150.0f, 15.0f * 10.0f, 21.0f * 10.0f);



     // Draw a portion of the texture (sprite)
     Engine::GRAPHICS->drawTextureRegion(nearestMonkeyImageId, 600.0f, 150.0f, 15.0f * 10.0f, 21.0f * 10.0f, 5.0f, 5.0f, 5.0f, 10.0f);
  };

  void Free() {};

  void Unload() {};
};