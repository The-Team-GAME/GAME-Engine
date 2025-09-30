#pragma once

#include "Precompiled.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <array>
#include "Shader.h"


class Graphics
{
public:
  // Add these for consistent virtual resolution
  static constexpr float VIRTUAL_WIDTH = 1600.0f;
  static constexpr float VIRTUAL_HEIGHT = 900.0f;

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

  struct SpriteSheet
  {
    unsigned int textureId;
    int frameWidth;
    int frameHeight;
    int totalFrames;
    float animationSpeed; // Frames per second
  };

  // Font rendering structures
  struct Character {
    glm::vec2 texCoords[4];  // UV coordinates in the atlas
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    unsigned int Advance;
  };

  struct Font {
    std::unordered_map<char, Character> Characters;
    unsigned int atlasTexture;  // Single texture for all characters
    unsigned int fontSize;
    float baseline;
    int atlasWidth, atlasHeight;
    bool isValid = true;  // Track if font is still valid
  };

  static const uint32_t MaxQuads = 10000;
  static const uint32_t MaxVertices = MaxQuads * 4;
  static const uint32_t MaxIndices = MaxQuads * 6;
  static const uint32_t MaxTextureSlots = 32; // Check your GPU's limit

  unsigned int quadVAO;
  unsigned int quadVBO;
  unsigned int quadIBO;

  uint32_t quadIndexCount = 0;
  std::vector<Vertex> quadVertexBufferBase;
  uint32_t vertexIndex = 0;

  // Rendering Data - Changed to unordered_map
  std::unordered_map<int, Texture> textures;
  std::unordered_map<int, Font> fonts;
  std::unordered_map<int, SpriteSheet> spriteSheets;

  // ID counters for generating unique IDs
  int nextTextureId = 0;
  int nextFontId = 0;
  int nextSpriteSheetId = 0;

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
  Graphics(int windowWidth, int windowHeight);
  ~Graphics();

  void beginFrame();
  void endFrame(); // New function to draw the batch

  void setViewport(int width, int height);

  // Camera controls
  void setCameraPosition(float x, float y);
  void setCameraZoom(float zoom);
  void setCameraRotation(float degrees);

  // Font and Text
  int loadFont(const char* filepath, unsigned int size);
  void unloadFont(int fontId);
  void unloadAllFonts();
  void drawText(int fontId, const std::string& text, float x, float y, float scale, const glm::vec4& color);

  void drawRect(const glm::vec4& color, float x, float y, float width, float height, float rotation = 0.0f, Origin origin = Origin::Center);
  void drawTexture(int imageId, float x, float y, float width, float height, float rotation = 0.0f, Origin origin = Origin::Center, const glm::vec4& tintColor = glm::vec4(1.0f));
  void drawTextureRegion(int imageId, float destX, float destY, float destWidth, float destHeight, float srcX, float srcY, float srcWidth, float srcHeight, float rotation = 0.0f, Origin origin = Origin::Center, const glm::vec4& tintColor = glm::vec4(1.0f));

  int loadTexture(const char* filepath, MinifyingFilter min_filter = MinifyingFilter::Linear);
  void unloadTexture(int textureId);
  void unloadAllTextures();
  void getTextureDimensions(int textureId, int& width, int& height) const;
  unsigned int getTextureHandle(int textureId) const; // Add this method

  void clearScreen(float r, float g, float b, float a);



  int loadSpriteSheet(const char* filepath, int frameWidth, int frameHeight, int totalFrames, float animationSpeed, MinifyingFilter min_filter = MinifyingFilter::Linear);
  void unloadSpriteSheet(int spriteSheetId);
  void unloadAllSpriteSheets();
  void drawAnimatedSprite(int spriteSheetId, float x, float y, float width, float height, float time, float rotation = 0.0f, Origin origin = Origin::Center, const glm::vec4& tintColor = glm::vec4(1.0f));

private:
  void startBatch();
  void nextBatch();
  void flush();
  glm::vec2 calculateOriginOffset(float width, float height, Origin origin) const;


  // Font data
  FT_Library ft;

public:
  glm::vec2 screenToVirtual(float screenX, float screenY) const;
};