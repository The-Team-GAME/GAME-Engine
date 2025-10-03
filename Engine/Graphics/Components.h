#pragma once

#include "Types.h"
#include <string>

namespace Components
{
  struct Texture
  {
    Engine::Graphics::Texture texture;

    Texture() : texture(nullptr) {}
    Texture(Engine::Graphics::Texture tex) : texture(tex) {}
  };

  struct TextureAtlas
  {
    std::vector<Engine::Graphics::SubTexture> subTextures;
    size_t drawIndex = 0; // Current subTexture index to draw

    // Constructor to initialize a texture atlas with evenly divided subTextures, works with multiple rows and columns
    TextureAtlas(int subTextureWidth, int subTextureHeight, size_t rows, size_t cols, size_t totalSubTextures = 0)
      : subTextures(), drawIndex(0)
    {
      // If totalSubTextures is 0, fill the entire atlas
      if (totalSubTextures == 0)
      {
        totalSubTextures = rows * cols;
      }
      subTextures.reserve(totalSubTextures);
      for (size_t row = 0; row < rows; ++row)
      {
        for (size_t col = 0; col < cols; ++col)
        {
          if (subTextures.size() >= totalSubTextures)
            break;

          float u0 = static_cast<float>(col * subTextureWidth) / (cols * subTextureWidth);
          float v0 = static_cast<float>(row * subTextureHeight) / (rows * subTextureHeight);
          float u1 = static_cast<float>((col + 1) * subTextureWidth) / (cols * subTextureWidth);
          float v1 = static_cast<float>((row + 1) * subTextureHeight) / (rows * subTextureHeight);

          Engine::Graphics::SubTexture subTexture;
          subTexture.texCoords = { glm::vec2(u0, v1), glm::vec2(u1, v1), glm::vec2(u1, v0), glm::vec2(u0, v0) };
          subTextures.push_back(subTexture);
        }
      }
    }
  };

  struct Animation
  {
    std::vector<Engine::Graphics::Frame> frames;
    float startTime = 0.0f;     // To get the elapsed time to calculate which frame to show
    float totalDuration = 0.0f; // Total duration of the animation

    // Evenly spaced frames
    Animation(size_t frameCount, float frameDuration) : frames(), startTime(0.0f), totalDuration(frameDuration* frameCount)
    {
      if (frameCount == 0)
        return;

      frames.resize(frameCount);
      for (size_t i = 0; i < frameCount; ++i)
      {
        frames[i].duration = frameDuration;
        frames[i].beforeTime = i * frameDuration;
      }
    }
  };

  struct Font
  {
    Engine::Graphics::Font font;

    Font() : font(nullptr) {}
    Font(Engine::Graphics::Font tex) : font(tex) {}
  };

  struct Text
  {
    std::string content;
  };

  struct Transform2D
  {
    glm::vec2 position;
    float rotation = 0.0f; // In degrees
    glm::vec2 scale = { 1.0f, 1.0f };
  };

}