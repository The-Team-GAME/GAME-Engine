#include "Precompiled.h"
#include <GL/glew.h>
#include <vector>
#include <iostream>
#include <numeric>
#include "Shader.h"
#include <filesystem>
#include "Graphics.h"

namespace
{
  // Anonymous namespace to keep these constants local to this file.

  // Default positions for the four vertices of a quad, centered at (0,0).
  // These will be transformed by a matrix for position, rotation, and scale.
  constexpr glm::vec4 QuadVertexPositions[] = {
      { -0.5f, -0.5f, 0.0f, 1.0f },
      {  0.5f, -0.5f, 0.0f, 1.0f },
      {  0.5f,  0.5f, 0.0f, 1.0f },
      { -0.5f,  0.5f, 0.0f, 1.0f }
  };

  // Default texture coordinates for a full texture.
  // These map the corners of the texture to the quad vertices.
  constexpr glm::vec2 TextureCoords[] = {
      { 0.0f, 0.0f }, // Bottom-left
      { 1.0f, 0.0f }, // Bottom-right
      { 1.0f, 1.0f }, // Top-right
      { 0.0f, 1.0f }  // Top-left
  };
}

// Recalculates the view matrix based on the camera's current position, rotation, and zoom.
void Graphics::updateViewMatrix() {
  view = glm::mat4(1.0f);
  view = glm::translate(view, glm::vec3(camera.position.x, camera.position.y, 0.0f));
  // Use virtual resolution for consistent camera behavior
  view = glm::translate(view, glm::vec3(VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f, 0.0f));
  view = glm::rotate(view, glm::radians(camera.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
  view = glm::scale(view, glm::vec3(camera.zoom, camera.zoom, 1.0f));
  view = glm::translate(view, glm::vec3(-VIRTUAL_WIDTH / 2.0f, -VIRTUAL_HEIGHT / 2.0f, 0.0f));
  view = glm::inverse(view);
  viewProjectionMatrix = projection * view;
}

// Initializes the graphics system, shaders, and buffers.
Graphics::Graphics(int windowWidth, int windowHeight)
{
  // Initialize FreeType
  if (FT_Init_FreeType(&ft))
  {
    std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
  }

  // Enable alpha blending.
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Load and compile the shader program.
  textureShader.init("Assets\\shaders\\texture.vert", "Assets\\shaders\\texture.frag");

  // Pre-allocate memory for the vertex buffer.
  quadVertexBufferBase.resize(MaxVertices);

  // Create and bind a Vertex Array Object (VAO) to store the vertex attribute configuration.
  glGenVertexArrays(1, &quadVAO);
  glBindVertexArray(quadVAO);

  // Create a Vertex Buffer Object (VBO) to hold vertex data.
  glGenBuffers(1, &quadVBO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  // Allocate memory on the GPU for the VBO, but don't upload any data yet (nullptr).
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * MaxVertices, nullptr, GL_DYNAMIC_DRAW);

  // Define the layout of the vertex data.
  // Attribute 0: Position (3 floats)
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Position));

  // Attribute 1: Color (4 floats)
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Color));

  // Attribute 2: Texture Coordinates (2 floats)
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, TexCoords));

  // Attribute 3: Texture ID (1 float)
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, TexID));

  // Generate indices for all possible quads in a batch.
  uint32_t indices[MaxIndices];
  uint32_t offset = 0;
  for (uint32_t i = 0; i < MaxIndices; i += 6)
  {
    // First triangle
    indices[i + 0] = offset + 0;
    indices[i + 1] = offset + 1;
    indices[i + 2] = offset + 2;
    // Second triangle
    indices[i + 3] = offset + 2;
    indices[i + 4] = offset + 3;
    indices[i + 5] = offset + 0;

    offset += 4; // Move to the next set of 4 vertices.
  }

  // Create an Index Buffer Object (IBO) and upload the index data.
  glGenBuffers(1, &quadIBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // Create a 1x1 white texture for drawing solid colors without a real texture.
  unsigned int whiteTexture;
  glGenTextures(1, &whiteTexture);
  glBindTexture(GL_TEXTURE_2D, whiteTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  uint32_t color = 0xffffffff; // White color in RGBA format
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color);

  // The white texture is always in slot 0.
  textureSlots[0] = whiteTexture;
  // Initialize other texture slots to 0.
  for (uint32_t i = 1; i < MaxTextureSlots; i++)
    textureSlots[i] = 0;

  // Set initial viewport and projection matrix.
  setViewport(windowWidth, windowHeight);

  // Use consistent virtual resolution
  projection = glm::ortho(0.0f, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, 0.0f, -1.0f, 1.0f);

  updateViewMatrix();
}

// Cleans up OpenGL objects and resources.
Graphics::~Graphics()
{
  textureShader.destroy();

  glDeleteVertexArrays(1, &quadVAO);
  glDeleteBuffers(1, &quadVBO);
  glDeleteBuffers(1, &quadIBO);

  for (const auto& tex : textures)
  {
    glDeleteTextures(1, &tex.id);
  }

  // Clean up all remaining fonts
  for (const auto& font : fonts)
  {
    glDeleteTextures(1, &font.atlasTexture);
  }

  for (const auto& texture : textureSlots)
  {
    if (texture != 0) glDeleteTextures(1, &texture);
  }

  FT_Done_FreeType(ft);
}

// Called at the beginning of a frame to prepare for drawing.
void Graphics::beginFrame()
{
  updateViewMatrix(); // Update camera matrix in case it moved.
  startBatch();       // Reset the batch renderer.
}

// Called at the end of a frame to draw everything that was batched.
void Graphics::endFrame()
{
  flush(); // Draw the current batch.
}

// Resets the batch data, preparing for a new set of draw calls.
void Graphics::startBatch()
{
  quadIndexCount = 0;
  vertexIndex = 0;
  textureSlotIndex = 1; // Slot 0 is reserved for the white texture.
}

// Draws the current batch and starts a new one.
void Graphics::nextBatch()
{
  flush();
  startBatch();
}

// Submits the current batch of vertices to the GPU for drawing.
void Graphics::flush()
{
  // Don't do anything if there's nothing to draw.
  if (quadIndexCount == 0)
    return;

  // Calculate the size of the vertex data to be uploaded.
  uint32_t dataSize = vertexIndex * sizeof(Vertex);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  // Upload the vertex data from our CPU-side vector to the GPU's VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, quadVertexBufferBase.data());

  // Prepare the shader for drawing.
  textureShader.use();
  // Set the combined view-projection matrix uniform.
  textureShader.setMatrix("u_ViewProjection", viewProjectionMatrix);

  // Create an array of sampler indices [0, 1, 2, ..., 31].
  int samplers[MaxTextureSlots];
  std::iota(samplers, samplers + MaxTextureSlots, 0);
  // Upload the sampler array to the shader.
  textureShader.setIntArray("u_Textures", MaxTextureSlots, samplers);

  // Bind all active textures to their respective texture units.
  for (uint32_t i = 0; i < textureSlotIndex; i++)
    glBindTextureUnit(i, textureSlots[i]);

  // Bind the VAO and draw the elements.
  glBindVertexArray(quadVAO);
  glDrawElements(GL_TRIANGLES, quadIndexCount, GL_UNSIGNED_INT, nullptr);
}

// Calculates the positional offset needed to draw a quad from a specific origin point.
glm::vec2 Graphics::calculateOriginOffset(float width, float height, Origin origin) const
{
  switch (origin)
  {
  case Origin::TopLeft: return { width / 2.0f, -height / 2.0f };
  case Origin::TopRight: return { -width / 2.0f, -height / 2.0f };
  case Origin::BottomLeft: return { width / 2.0f, height / 2.0f };
  case Origin::BottomRight: return { -width / 2.0f, height / 2.0f };
  default: return { 0.0f, 0.0f }; // Center (default)
  }
}

// Sets the OpenGL viewport and updates the projection matrix.
void Graphics::setViewport(int width, int height)
{
  viewportWidth = width;
  viewportHeight = height;

  // Tell OpenGL the size of the rendering window.
  glViewport(0, 0, width, height);

  // Update the orthographic projection matrix to match the new dimensions.
  // projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);

  updateViewMatrix(); // Recalculate the combined matrix.
}

// Sets the camera's world position.
void Graphics::setCameraPosition(float x, float y)
{
  camera.position = { x, y };
  updateViewMatrix();
}

// Sets the camera's zoom level.
void Graphics::setCameraZoom(float zoom)
{
  camera.zoom = zoom;
  updateViewMatrix();
}

// Sets the camera's rotation in degrees.
void Graphics::setCameraRotation(float degrees)
{
  camera.rotation = degrees;
  updateViewMatrix();
}

// Adds a colored rectangle to the current drawing batch.
void Graphics::drawRect(const glm::vec4& color, float x, float y, float width, float height, float rotation, Origin origin) {
  // If the current batch is full, draw it and start a new one.
  if (quadIndexCount >= MaxIndices)
  {
    nextBatch();
  }

  // Calculate the origin offset.
  glm::vec2 offset = calculateOriginOffset(width, height, origin);

  // Create the transformation matrix for the quad.
  glm::mat4 transform = glm::translate(glm::mat4(1.0f), { x + offset.x, y + offset.y, 0.0f })
    * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
    * glm::scale(glm::mat4(1.0f), { width, height, 1.0f });

  // Use texture ID 0.0f, which corresponds to the white texture slot.
  constexpr float texID = 0.0f;

  // Add the 4 vertices of the quad to the vertex buffer.
  quadVertexBufferBase[vertexIndex++] = { transform * QuadVertexPositions[0], color, {0.0f, 0.0f}, texID };
  quadVertexBufferBase[vertexIndex++] = { transform * QuadVertexPositions[1], color, {1.0f, 0.0f}, texID };
  quadVertexBufferBase[vertexIndex++] = { transform * QuadVertexPositions[2], color, {1.0f, 1.0f}, texID };
  quadVertexBufferBase[vertexIndex++] = { transform * QuadVertexPositions[3], color, {0.0f, 1.0f}, texID };

  // Increment the index count for the two triangles that make up the quad.
  quadIndexCount += 6;
}

// Loads a font from a file and prepares it for rendering.
int Graphics::loadFont(const char* filepath, unsigned int size)
{
  FT_Face face;
  if (FT_New_Face(ft, filepath, 0, &face))
  {
    std::cout << "ERROR::FREETYPE: Failed to load font: " << filepath << std::endl;
    return -1;
  }

  FT_Set_Pixel_Sizes(face, 0, size);

  // Calculate atlas dimensions
  int atlasWidth = 0, atlasHeight = 0;

  // First pass: calculate required atlas size
  for (unsigned char c = 0; c < 128; c++)
  {
    if (FT_Load_Char(face, c, FT_LOAD_RENDER))
      continue;

    atlasWidth += face->glyph->bitmap.width + 1; // +1 for padding
    atlasHeight = std::max(atlasHeight, static_cast<int>(face->glyph->bitmap.rows));
  }

  // Create RGBA atlas data
  std::vector<unsigned char> atlasData(atlasWidth * atlasHeight * 4, 0);

  // Create atlas texture
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  unsigned int atlasTexture;
  glGenTextures(1, &atlasTexture);
  glBindTexture(GL_TEXTURE_2D, atlasTexture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  Font font;
  font.fontSize = size;
  font.baseline = 0.0f;
  font.atlasTexture = atlasTexture;
  font.atlasWidth = atlasWidth;
  font.atlasHeight = atlasHeight;
  font.isValid = true;

  // Second pass: pack characters into atlas
  int currentX = 0;
  for (unsigned char c = 0; c < 128; c++)
  {
    if (FT_Load_Char(face, c, FT_LOAD_RENDER))
      continue;

    // Copy glyph data to RGBA atlas
    if (face->glyph->bitmap.width > 0 && face->glyph->bitmap.rows > 0)
    {
      for (int y = 0; y < face->glyph->bitmap.rows; y++)
      {
        for (int x = 0; x < face->glyph->bitmap.width; x++)
        {
          int atlasIndex = ((y * atlasWidth) + (currentX + x)) * 4;
          int glyphIndex = y * face->glyph->bitmap.width + x;

          unsigned char alpha = face->glyph->bitmap.buffer[glyphIndex];

          // Set RGBA: white color with glyph alpha
          atlasData[atlasIndex + 0] = 255; // R
          atlasData[atlasIndex + 1] = 255; // G  
          atlasData[atlasIndex + 2] = 255; // B
          atlasData[atlasIndex + 3] = alpha; // A
        }
      }
    }

    // Calculate UV coordinates
    float x0 = static_cast<float>(currentX) / atlasWidth;
    float x1 = static_cast<float>(currentX + face->glyph->bitmap.width) / atlasWidth;
    float y0 = 0.0f;
    float y1 = static_cast<float>(face->glyph->bitmap.rows) / atlasHeight;

    Character character = {
        { {x0, y0}, {x1, y0}, {x1, y1}, {x0, y1} }, // UV coords
        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
        static_cast<unsigned int>(face->glyph->advance.x)
    };

    font.Characters.insert(std::pair<char, Character>(c, character));
    font.baseline = std::max(font.baseline, static_cast<float>(character.Bearing.y));

    currentX += face->glyph->bitmap.width + 1; // +1 for padding
  }

  // Upload the complete RGBA atlas to GPU
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasWidth, atlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlasData.data());

  FT_Done_Face(face);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

  fonts.push_back(font);
  return static_cast<int>(fonts.size() - 1);
}

// Unloads a font by its ID
void Graphics::unloadFont(int fontId)
{
  if (fontId < 0 || fontId >= static_cast<int>(fonts.size()))
  {
    std::cout << "Graphics: Invalid font ID: " << fontId << std::endl;
    return;
  }

  Font& font = fonts[fontId];

  // Delete the atlas texture
  glDeleteTextures(1, &font.atlasTexture);

  fonts.erase(fonts.begin() + fontId);

  std::cout << "Graphics: Unloaded font with ID " << fontId << std::endl;
}

// Unloads all fonts
void Graphics::unloadAllFonts()
{
  for (int i = 0; i < static_cast<int>(fonts.size()); ++i)
  {
    if (fonts[i].isValid)
    {
      unloadFont(i);
    }
  }
  std::cout << "Graphics: Unloaded all fonts" << std::endl;
}

// Draws text on the screen.
void Graphics::drawText(int fontId, const std::string& text, float x, float y, float scale, const glm::vec4& color)
{
  if (fontId < 0 || fontId >= static_cast<int>(fonts.size()))
  {
    std::cout << "Graphics: Invalid font ID: " << fontId << std::endl;
    return;
  }

  const Font& font = fonts[fontId];

  // Find or add font atlas to texture slots
  float textureIndex = 0.0f;
  auto texIt = std::find(textureSlots.begin() + 1, textureSlots.begin() + textureSlotIndex, font.atlasTexture);
  if (texIt != textureSlots.begin() + textureSlotIndex)
  {
    textureIndex = static_cast<float>(std::distance(textureSlots.begin(), texIt));
  }
  else
  {
    if (textureSlotIndex >= MaxTextureSlots)
    {
      nextBatch();
      textureIndex = 1.0f;
    }
    else
    {
      textureIndex = static_cast<float>(textureSlotIndex);
    }
    textureSlots[static_cast<int>(textureIndex)] = font.atlasTexture;
    if (textureSlotIndex < MaxTextureSlots) textureSlotIndex++;
  }

  float currentX = x;

  for (char c : text)
  {
    auto it = font.Characters.find(c);
    if (it == font.Characters.end())
      continue;

    const Character& ch = it->second;

    if (ch.Size.x > 0 && ch.Size.y > 0)
    {
      if (quadIndexCount >= MaxIndices)
        nextBatch();

      float xpos = currentX + ch.Bearing.x * scale;
      float ypos = y + (font.baseline - ch.Bearing.y) * scale;
      float w = ch.Size.x * scale;
      float h = ch.Size.y * scale;

      glm::mat4 transform = glm::translate(glm::mat4(1.0f), { xpos + w / 2.0f, ypos + h / 2.0f, 0.0f })
        * glm::scale(glm::mat4(1.0f), { w, h, 1.0f });

      // Use pre-calculated UV coordinates
      quadVertexBufferBase[vertexIndex++] = { transform * QuadVertexPositions[0], color, ch.texCoords[0], textureIndex };
      quadVertexBufferBase[vertexIndex++] = { transform * QuadVertexPositions[1], color, ch.texCoords[1], textureIndex };
      quadVertexBufferBase[vertexIndex++] = { transform * QuadVertexPositions[2], color, ch.texCoords[2], textureIndex };
      quadVertexBufferBase[vertexIndex++] = { transform * QuadVertexPositions[3], color, ch.texCoords[3], textureIndex };

      quadIndexCount += 6;
    }

    currentX += (ch.Advance >> 6) * scale;
  }
}

// Loads a texture from a file and returns an ID for it.
int Graphics::loadTexture(const char* filepath, MinifyingFilter min_filter)
{
  int width, height, nrChannels;
  // Load image data from file using stb_image.
  unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);

  if (!data)
  {
    std::cout << "Failed to load texture: " << filepath << std::endl;
    return -1; // Return -1 on failure.
  }

  // Create an OpenGL texture object.
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  // Set texture wrapping parameters.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Set texture filtering parameters.
  if (min_filter == MinifyingFilter::Linear) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  else if (min_filter == MinifyingFilter::Nearest) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  // Determine the image format (RGB or RGBA).
  GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
  // Upload the image data to the GPU.
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  // Free the CPU-side image data.
  stbi_image_free(data);

  // Store the texture info and return its ID (index in the vector).
  Texture tex = { texture, width, height };
  textures.push_back(tex);

  return static_cast<int>(textures.size() - 1);
}

// A convenience function to draw a full texture.
void Graphics::drawTexture(int imageId, float x, float y, float width, float height, float rotation, Origin origin, const glm::vec4& tintColor)
{
  if (imageId < 0 || imageId >= static_cast<int>(textures.size()))
  {
    std::cout << "Invalid image ID: " << imageId << std::endl;
    return;
  }
  // Calls drawTextureRegion with the full dimensions of the source texture.
  drawTextureRegion(
    imageId,
    x, y, width, height,
    0, 0, static_cast<float>(textures[imageId].width), static_cast<float>(textures[imageId].height),
    rotation,
    origin,
    tintColor
  );
}

// Adds a region of a texture to the current drawing batch.
void Graphics::drawTextureRegion(int imageId, float destX, float destY, float destWidth, float destHeight, float srcX, float srcY, float srcWidth, float srcHeight, float rotation, Origin origin, const glm::vec4& tintColor)
{
  if (imageId < 0 || imageId >= static_cast<int>(textures.size()))
  {
    std::cout << "Invalid image ID: " << imageId << std::endl;
    return;
  }

  // If the batch is full (either too many indices or too many textures), start a new one.
  if (quadIndexCount >= MaxIndices || textureSlotIndex >= MaxTextureSlots)
  {
    nextBatch();
  }

  float textureIndex = 0.0f;
  const auto& tex = textures[imageId];

  // Find if the texture is already in one of the active slots for this batch.
  auto it = std::find(textureSlots.begin() + 1, textureSlots.begin() + textureSlotIndex, tex.id);
  if (it != textureSlots.begin() + textureSlotIndex)
  {
    // If found, get its index.
    textureIndex = static_cast<float>(std::distance(textureSlots.begin(), it));
  }
  else
  {
    // If not found, add it to the next available slot.
    textureIndex = (float)textureSlotIndex;
    textureSlots[textureSlotIndex] = tex.id;
    textureSlotIndex++;
  }

  // Calculate origin offset.
  glm::vec2 offset = calculateOriginOffset(destWidth, destHeight, origin);

  // Create the transformation matrix for the destination quad.
  glm::mat4 transform = glm::translate(glm::mat4(1.0f), { destX + offset.x, destY + offset.y, 0.0f })
    * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
    * glm::scale(glm::mat4(1.0f), { destWidth, destHeight, 1.0f });

  // Calculate the texture coordinates for the source region.
  glm::vec2 texCoords[] = {
      { srcX / tex.width, srcY / tex.height },
      { (srcX + srcWidth) / tex.width, srcY / tex.height },
      { (srcX + srcWidth) / tex.width, (srcY + srcHeight) / tex.height },
      { srcX / tex.width, (srcY + srcHeight) / tex.height }
  };

  // Add the 4 vertices of the quad to the vertex buffer.
  quadVertexBufferBase[vertexIndex++] = { transform * QuadVertexPositions[0], tintColor, texCoords[0], textureIndex };
  quadVertexBufferBase[vertexIndex++] = { transform * QuadVertexPositions[1], tintColor, texCoords[1], textureIndex };
  quadVertexBufferBase[vertexIndex++] = { transform * QuadVertexPositions[2], tintColor, texCoords[2], textureIndex };
  quadVertexBufferBase[vertexIndex++] = { transform * QuadVertexPositions[3], tintColor, texCoords[3], textureIndex };

  quadIndexCount += 6;
}

// Loads a sprite sheet from a file and returns an ID for it.
int Graphics::loadSpriteSheet(const char* filepath, int frameWidth, int frameHeight, int totalFrames, float animationSpeed, MinifyingFilter min_filter)
{
  // Load the texture using existing loadTexture function
  int textureId = loadTexture(filepath, min_filter);

  if (textureId == -1)
  {
    std::cout << "Failed to load sprite sheet: " << filepath << std::endl;
    return -1;
  }

  // Create sprite sheet data
  SpriteSheet spriteSheet = {
    static_cast<unsigned int>(textureId),
    frameWidth,
    frameHeight,
    totalFrames,
    animationSpeed
  };

  spriteSheets.push_back(spriteSheet);
  return static_cast<int>(spriteSheets.size() - 1);
}

// Draws an animated sprite from a sprite sheet.
void Graphics::drawAnimatedSprite(int spriteSheetId, float x, float y, float width, float height, float time, float rotation, Origin origin, const glm::vec4& tintColor)
{
  if (spriteSheetId < 0 || spriteSheetId >= static_cast<int>(spriteSheets.size()))
  {
    std::cout << "Invalid sprite sheet ID: " << spriteSheetId << std::endl;
    return;
  }

  const SpriteSheet& spriteSheet = spriteSheets[spriteSheetId];

  // Calculate current frame based on time and animation speed
  int currentFrame = static_cast<int>(time * spriteSheet.animationSpeed) % spriteSheet.totalFrames;

  // Get texture dimensions to calculate frames per row
  const Texture& texture = textures[spriteSheet.textureId];
  int framesPerRow = texture.width / spriteSheet.frameWidth;

  // Calculate source coordinates for current frame
  int frameX = currentFrame % framesPerRow;
  int frameY = currentFrame / framesPerRow;

  float srcX = static_cast<float>(frameX * spriteSheet.frameWidth);
  float srcY = static_cast<float>(frameY * spriteSheet.frameHeight);

  // Use drawTextureRegion to render the specific frame
  drawTextureRegion(
    spriteSheet.textureId,
    x, y, width, height,
    srcX, srcY, static_cast<float>(spriteSheet.frameWidth), static_cast<float>(spriteSheet.frameHeight),
    rotation,
    origin,
    tintColor
  );
}

// Clears the screen with a specified color.
void Graphics::clearScreen(float r, float g, float b, float a)
{
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);
}

glm::vec2 Graphics::screenToVirtual(float screenX, float screenY) const
{
  return {
    (screenX / viewportWidth) * VIRTUAL_WIDTH,
    (screenY / viewportHeight) * VIRTUAL_HEIGHT
  };
}

// Unloads a texture by its ID
void Graphics::unloadTexture(int textureId)
{
  if (textureId < 0 || textureId >= static_cast<int>(textures.size()))
  {
    std::cout << "Graphics: Invalid texture ID: " << textureId << std::endl;
    return;
  }

  // Delete the OpenGL texture object
  glDeleteTextures(1, &textures[textureId].id);

  // Remove from the textures vector
  textures.erase(textures.begin() + textureId);

  // Clear the texture from any active texture slots
  for (uint32_t i = 0; i < MaxTextureSlots; i++)
  {
    if (textureSlots[i] == textures[textureId].id)
    {
      textureSlots[i] = 0;
    }
  }

  std::cout << "Graphics: Unloaded texture with ID " << textureId << std::endl;
}

// Unloads all textures
void Graphics::unloadAllTextures()
{
  for (const auto& texture : textures)
  {
    glDeleteTextures(1, &texture.id);
  }

  textures.clear();

  // Clear all texture slots except the white texture (slot 0)
  for (uint32_t i = 1; i < MaxTextureSlots; i++)
  {
    textureSlots[i] = 0;
  }

  textureSlotIndex = 1; // Reset to 1 (slot 0 is reserved for white texture)

  std::cout << "Graphics: Unloaded all textures" << std::endl;
}

// Unloads a sprite sheet by its ID
void Graphics::unloadSpriteSheet(int spriteSheetId)
{
  if (spriteSheetId < 0 || spriteSheetId >= static_cast<int>(spriteSheets.size()))
  {
    std::cout << "Graphics: Invalid sprite sheet ID: " << spriteSheetId << std::endl;
    return;
  }

  // Note: We don't delete the underlying texture here because it's managed
  // by the textures vector. The sprite sheet just references a texture ID.
  // If you want to delete the underlying texture as well, you would call:
  // unloadTexture(spriteSheets[spriteSheetId].textureId);

  spriteSheets.erase(spriteSheets.begin() + spriteSheetId);

  std::cout << "Graphics: Unloaded sprite sheet with ID " << spriteSheetId << std::endl;
}

// Unloads all sprite sheets
void Graphics::unloadAllSpriteSheets()
{
  spriteSheets.clear();
}
