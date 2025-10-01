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
#include <iostream>
#include "Graphics.h"

class DevSpriteAnimationState : public Engine::GameState<DevSpriteAnimationState> {
private:
  s32 marioSpriteSheetId = -1;
  f32 animationTime = 0.0f;

public:
  void Load() {};

  void Init() {
    // Load sprite sheet using ResourceManager
    marioSpriteSheetId = Engine::RESOURCES.load_sprite_sheet(
      "mario_sprite",
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
    Engine::GRAPHICS.clear_screen(0.1f, 0.1f, 0.1f, 1.0f);

    // Draw animated sprite
    Engine::GRAPHICS.draw_animated_sprite(marioSpriteSheetId, 200.0f, 150.0f, 96.0f, 96.0f, animationTime);
  };

  void Free() {};

  void Unload() {
    // Unload sprite sheet using ResourceManager
    Engine::RESOURCES.unload_sprite_sheet("mario_sprite");
  };
};