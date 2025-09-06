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
  inline static unsigned int quadVAO;
  inline static std::vector<Texture> textures;

  inline static Shader basicShader;
  inline static Shader textureShader;
  inline static glm::mat4 projection;

  static void setupRectangle();

public:
  static void init();
  static void exit();

  static void drawRect(float x, float y, float width, float height, glm::vec4 color);

  static unsigned int loadImage(const char* filepath, MinifyingFilter min_filter = MinifyingFilter::Linear);
  static void drawImage(int imageId, float x, float y, float width, float height);


  static void clearScreen(float r, float g, float b, float a);
};