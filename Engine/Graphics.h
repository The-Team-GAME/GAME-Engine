#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Shader.h"
#include <array>


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

  enum class Origin
  {
    Center,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
  };

private:
  struct Vertex
  {
    glm::vec3 Position;
    glm::vec4 Color;
    glm::vec2 TexCoords;
    float TexID;
  };

  static const uint32_t MaxQuads = 10000;
  static const uint32_t MaxVertices = MaxQuads * 4;
  static const uint32_t MaxIndices = MaxQuads * 6;
  static const uint32_t MaxTextureSlots = 32; // Check your GPU's limit

  unsigned int quadVAO;
  unsigned int quadVBO;
  unsigned int quadIBO;

  uint32_t quadIndexCount = 0;
  Vertex* quadVertexBufferBase = nullptr;
  Vertex* quadVertexBufferPtr = nullptr;

  std::vector<Texture> textures;
  std::array<unsigned int, MaxTextureSlots> textureSlots;
  uint32_t textureSlotIndex = 1; // 0 = white texture

  Shader textureShader;
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 viewProjectionMatrix;
  Camera2D camera;

  int viewportWidth = 800;
  int viewportHeight = 600;

  void updateViewMatrix();

public:
  void init();
  void exit();

  void beginFrame();
  void endFrame(); // New function to draw the batch

  void setViewport(int width, int height);

  // Camera controls
  void setCameraPosition(float x, float y);
  void setCameraZoom(float zoom);
  void setCameraRotation(float degrees);
  const Camera2D& getCamera() const { return camera; }

  void drawRect(const glm::vec4& color, float x, float y, float width, float height, float rotation = 0.0f, Origin origin = Origin::Center);
  void drawTexture(int imageId, float x, float y, float width, float height, float rotation = 0.0f, Origin origin = Origin::Center, const glm::vec4& tintColor = glm::vec4(1.0f));
  void drawTextureRegion(int imageId, float destX, float destY, float destWidth, float destHeight, float srcX, float srcY, float srcWidth, float srcHeight, float rotation = 0.0f, Origin origin = Origin::Center, const glm::vec4& tintColor = glm::vec4(1.0f));

  int loadTexture(const char* filepath, MinifyingFilter min_filter = MinifyingFilter::Linear);
  void clearScreen(float r, float g, float b, float a);

private:
  void startBatch();
  void nextBatch();
  void flush();
  glm::vec2 calculateOriginOffset(float width, float height, Origin origin) const;
};