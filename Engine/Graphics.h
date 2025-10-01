#pragma once

#include "Precompiled.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <array>
#include "Shader.h"
#include "TypeDef.h"

class Graphics
{
private:
  s32 win_width;
  s32 win_height;
public:
  // Add these for consistent virtual resolution
  static constexpr f32 VIRTUAL_WIDTH = 1600.0f;
  static constexpr f32 VIRTUAL_HEIGHT = 900.0f;

  struct Texture
  {
    u32 id;
    s32 width;
    s32 height;
  };
  struct Camera2D
  {
    glm::vec2 position = { 0.0f, 0.0f };
    f32 zoom = 1.0f;
    f32 rotation = 0.0f;
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
    f32 TexID;
  };

  struct SpriteSheet
  {
    u32 texture_id;
    s32 frameWidth;
    s32 frameHeight;
    s32 totalFrames;
    f32 animationSpeed; // Frames per second
  };

  // Font rendering structures
  struct Character {
    glm::vec2 texCoords[4];  // UV coordinates in the atlas
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    u32 Advance;
  };

  struct Font {
    std::unordered_map<char, Character> Characters;
    u32 atlasTexture;  // Single texture for all characters
    u32 fontSize;
    f32 baseline;
    s32 atlasWidth, atlasHeight;
    bool isValid = true;  // Track if font is still valid
  };

  static const u32 MaxQuads = 10000;
  static const u32 MaxVertices = MaxQuads * 4;
  static const u32 MaxIndices = MaxQuads * 6;
  static const u32 MaxTextureSlots = 32; // Check your GPU's limit

  u32 quadVAO;
  u32 quadVBO;
  u32 quadIBO;

  u32 quadIndexCount = 0;
  std::vector<Vertex> quadVertexBufferBase;
  u32 vertexIndex = 0;

  // Rendering Data - Changed to unordered_map
  std::unordered_map<s32, Texture> textures;
  std::unordered_map<s32, Font> fonts;
  std::unordered_map<s32, SpriteSheet> spriteSheets;

  // ID counters for generating unique IDs
  s32 nextTextureId = 0;
  s32 nextFontId = 0;
  s32 nextSpriteSheetId = 0;

  std::array<u32, MaxTextureSlots> textureSlots;
  u32 textureSlotIndex = 1; // 0 = white texture


  Shader textureShader;
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 viewProjectionMatrix;
  Camera2D camera;

  s32 viewportWidth = 800;
  s32 viewportHeight = 600;

  void updateViewMatrix();

public:
  Graphics();
  void set_params(s32 win_width, s32 win_height);
  void init_graphics();
  ~Graphics();

  void beginFrame();
  void endFrame(); // New function to draw the batch

  void setViewport(s32 width, s32 height);

  // Camera controls
  void set_camera_position(f32 x, f32 y);
  void set_camera_zoom(f32 zoom);
  void set_camera_rotation(f32 degrees);

  // Font and Text
  s32 loadFont(const char* filepath, u32 size);
  void unloadFont(s32 fontId);
  void unloadAllFonts();
  void drawText(s32 fontId, const std::string& text, f32 x, f32 y, f32 scale, const glm::vec4& color);

  void drawRect(const glm::vec4& color, f32 x, f32 y, f32 width, f32 height, f32 rotation = 0.0f, Origin origin = Origin::Center);
  void drawCircle(const glm::vec4& color, f32 x, f32 y, f32 radius, s32 segments = 32);
  void drawLine(const glm::vec4& color, f32 x1, f32 y1, f32 x2, f32 y2, f32 thickness = 2.0f);
  void drawTexture(s32 imageId, f32 x, f32 y, f32 width, f32 height, f32 rotation = 0.0f, Origin origin = Origin::Center, const glm::vec4& ts32Color = glm::vec4(1.0f));
  void drawTextureRegion(s32 imageId, f32 destX, f32 destY, f32 destWidth, f32 destHeight, f32 srcX, f32 srcY, f32 srcWidth, f32 srcHeight, f32 rotation = 0.0f, Origin origin = Origin::Center, const glm::vec4& ts32Color = glm::vec4(1.0f));

  s32 loadTexture(const char* filepath, MinifyingFilter min_filter = MinifyingFilter::Linear);
  void unloadTexture(s32 texture_id);
  void unloadAllTextures();
  void getTextureDimensions(s32 texture_id, s32& width, s32& height) const;
  u32 getTextureHandle(s32 texture_id) const; // Add this method

  void clearScreen(f32 r, f32 g, f32 b, f32 a);



  s32 loadSpriteSheet(const char* filepath, s32 frameWidth, s32 frameHeight, s32 totalFrames, f32 animationSpeed, MinifyingFilter min_filter = MinifyingFilter::Linear);
  void unloadSpriteSheet(s32 spriteSheetId);
  void unloadAllSpriteSheets();
  void drawAnimatedSprite(s32 spriteSheetId, f32 x, f32 y, f32 width, f32 height, f32 time, f32 rotation = 0.0f, Origin origin = Origin::Center, const glm::vec4& ts32Color = glm::vec4(1.0f));

private:
  void startBatch();
  void nextBatch();
  void flush();
  glm::vec2 calculateOriginOffset(f32 width, f32 height, Origin origin) const;


  // Font data
  FT_Library ft;

public:
  glm::vec2 screenToVirtual(f32 screenX, f32 screenY) const;
};