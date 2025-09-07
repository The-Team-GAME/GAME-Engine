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

  unsigned int loadImage(const char* filepath, MinifyingFilter min_filter = MinifyingFilter::Linear);
  void drawImage(int imageId, float x, float y, float width, float height);


  void clearScreen(float r, float g, float b, float a);
};