#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Shader.h"


class Graphics
{
public:

  struct Texture
  {
    unsigned int id;
    int width;
    int height;
  };

  enum class MinifyingFilter
  {
    Nearest,
    Linear
  };

private:
  unsigned int quadVAO;
  std::vector<Texture> textures;

  Shader basicShader;
  Shader textureShader;
  glm::mat4 projection;

  void setupRectangle();

public:
  void init();
  void exit();

  void drawRect(float x, float y, float width, float height, glm::vec4 color);

  int loadTexture(const char* filepath, MinifyingFilter min_filter = MinifyingFilter::Linear);
  void drawTexture(int imageId, float x, float y, float width, float height, float rotation = 0.0f);
  void drawTextureRegion(int imageId, float destX, float destY, float destWidth, float destHeight, float srcX, float srcY, float srcWidth, float srcHeight, float rotation = 0.0f);

  void clearScreen(float r, float g, float b, float a);
};