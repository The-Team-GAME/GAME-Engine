#pragma once

#include "StateManager.h"
#include "core.h"

class SpriteState : public Engine::GameState<SpriteState>
{
private:
  int linearMonkeyImageId = -1;
  int nearestMonkeyImageId = -1;

  float spritePosX = 0.0f;

public:
  void Load() {};

  void Init()
  {
    linearMonkeyImageId = Engine::GRAPHICS->loadTexture("Assets/images/monkey.png");
    nearestMonkeyImageId = Engine::GRAPHICS->loadTexture("Assets/images/monkey.png", Graphics::MinifyingFilter::Nearest); // when loading pixel art, use nearest filter to avoid blurriness
  };

  void Update() {};

  void Draw()
  {
    Engine::GRAPHICS->clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background

    Engine::GRAPHICS->drawTexture(linearMonkeyImageId, 200.0f, 150.0f, 150.0f, 210.0f);
    Engine::GRAPHICS->drawTexture(nearestMonkeyImageId, 400.0f, 150.0f, 150.0f, 210.0f);

    // Draw a portion of the texture (sprite)
    Engine::GRAPHICS->drawTextureRegion(
      nearestMonkeyImageId,
      600.0f, // destX
      150.0f, // destY
      150.0f, // destWidth
      210.0f, // destHeight
      spritePosX += Engine::CORE->getDt() * 10.0f, // srcX
      0.0f, // srcY
      15.0f, // srcWidth
      21.0f // srcHeight
    );
  };

  void Free() {};

  void Unload() {};
};