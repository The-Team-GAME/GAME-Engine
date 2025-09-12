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

  struct Camera2D
  {
    glm::vec2 position = { 0.0f, 0.0f };
    float zoom = 1.0f;
    float rotation = 0.0f;
  };

  enum class MinifyingFilter
  {
    Nearest,
    Linear
  };

private:
  unsigned int quadVAO;
  unsigned int quadVBO;  // Add this to store VBO
  std::vector<Texture> textures;

  Shader textureShader;
  glm::mat4 projection;
  glm::mat4 view;
  Camera2D camera;

  int viewportWidth = 800;
  int viewportHeight = 600;

  void setupRectangle();
  void updateViewMatrix();
  void applyModelTransform(glm::mat4& model, float x, float y, float width, float height, float rotation);  // Helper for draw functions

public:
  void init();
  void exit();

  void beginFrame();

  void setViewport(int width, int height);

  // Camera controls
  void setCameraPosition(float x, float y);
  void setCameraZoom(float zoom);
  void setCameraRotation(float degrees);
  const Camera2D& getCamera() const { return camera; }

  // TODO: change color to be first parameter
  // TODO: color should use our own math lib
  void drawRect(float x, float y, float width, float height, glm::vec4 color, float rotation = 0.0f);

  int loadTexture(const char* filepath, MinifyingFilter min_filter = MinifyingFilter::Linear);
  void drawTexture(int imageId, float x, float y, float width, float height, float rotation = 0.0f);
  void drawTextureRegion(int imageId, float destX, float destY, float destWidth, float destHeight, float srcX, float srcY, float srcWidth, float srcHeight, float rotation = 0.0f);

  void clearScreen(float r, float g, float b, float a);
};