#include "Precompiled.h"
#include <GL/glew.h>
#include <vector>
#include <iostream>
#include <numeric>
#include "Shader.h"
#include <filesystem>
#include "Graphics.h"
#include "Core.h"

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
  view = glm::scale(view, glm::vec3(1.0f / camera.zoom, 1.0f / camera.zoom, 1.0f)); // Changed: invert the zoom
  view = glm::translate(view, glm::vec3(-VIRTUAL_WIDTH / 2.0f, -VIRTUAL_HEIGHT / 2.0f, 0.0f));
  view = glm::inverse(view);
  viewProjectionMatrix = projection * view;
}

// Initializes the graphics system, shaders, and buffers.
Graphics::Graphics()
{
  // empoty by design
}


void Graphics::set_params(int width, int height)
{
  win_width = width;
  win_height = height;
}

void Graphics::init_graphics()
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
  setViewport(win_width, win_height);

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

  for (const auto& pair : textures)
  {
    glDeleteTextures(1, &pair.second.id);
  }

  // Clean up all remaining fonts
  for (const auto& pair : fonts)
  {
    glDeleteTextures(1, &pair.second.atlasTexture);
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
  Engine::PERFORMANCE.start("Graphics");
  updateViewMatrix(); // Update camera matrix in case it moved.
  startBatch();       // Reset the batch renderer.
}

// Called at the end of a frame to draw everything that was batched.
void Graphics::endFrame()
{
  Engine::PERFORMANCE.stop("Graphics");
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

// Draws a filled circle using a triangle fan approach (batched as quads approximation)
void Graphics::drawCircle(const glm::vec4& color, float x, float y, float radius, int segments)
{
  // Clamp segments to reasonable values
  segments = std::max(8, std::min(segments, 64));

  // We'll approximate the circle using triangular quads
  // Each quad represents a slice of the circle
  constexpr float texID = 0.0f; // Use white texture

  float angleStep = 2.0f * 3.14159265359f / segments;

  for (int i = 0; i < segments; i++)
  {
    if (quadIndexCount >= MaxIndices)
    {
      nextBatch();
    }

    float angle1 = i * angleStep;
    float angle2 = (i + 1) * angleStep;

    // Create a triangle as a degenerate quad (center + 2 edge points + duplicate)
    glm::vec3 center = { x, y, 0.0f };
    glm::vec3 p1 = { x + radius * cosf(angle1), y + radius * sinf(angle1), 0.0f };
    glm::vec3 p2 = { x + radius * cosf(angle2), y + radius * sinf(angle2), 0.0f };

    quadVertexBufferBase[vertexIndex++] = { center, color, {0.5f, 0.5f}, texID };
    quadVertexBufferBase[vertexIndex++] = { p1, color, {0.5f, 0.5f}, texID };
    quadVertexBufferBase[vertexIndex++] = { p2, color, {0.5f, 0.5f}, texID };
    quadVertexBufferBase[vertexIndex++] = { p2, color, {0.5f, 0.5f}, texID }; // Duplicate to form quad

    quadIndexCount += 6;
  }
}

// Draws a line between two points with specified thickness
void Graphics::drawLine(const glm::vec4& color, float x1, float y1, float x2, float y2, float thickness)
{
  if (quadIndexCount >= MaxIndices)
  {
    nextBatch();
  }

  // Calculate the line direction and perpendicular offset
  float dx = x2 - x1;
  float dy = y2 - y1;
  float length = sqrtf(dx * dx + dy * dy);

  if (length < 0.001f) return; // Don't draw zero-length lines

  // Normalize direction
  dx /= length;
  dy /= length;

  // Perpendicular vector for thickness
  float perpX = -dy * thickness * 0.5f;
  float perpY = dx * thickness * 0.5f;

  constexpr float texID = 0.0f; // Use white texture

  // Create a quad representing the line
  glm::vec3 v0 = { x1 + perpX, y1 + perpY, 0.0f };
  glm::vec3 v1 = { x2 + perpX, y2 + perpY, 0.0f };
  glm::vec3 v2 = { x2 - perpX, y2 - perpY, 0.0f };
  glm::vec3 v3 = { x1 - perpX, y1 - perpY, 0.0f };

  quadVertexBufferBase[vertexIndex++] = { v0, color, {0.0f, 0.0f}, texID };
  quadVertexBufferBase[vertexIndex++] = { v1, color, {1.0f, 0.0f}, texID };
  quadVertexBufferBase[vertexIndex++] = { v2, color, {1.0f, 1.0f}, texID };
  quadVertexBufferBase[vertexIndex++] = { v3, color, {0.0f, 1.0f}, texID };

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

  int fontId = nextFontId++;
  fonts[fontId] = font;
  return fontId;
}

// Unloads a font by its ID
void Graphics::unloadFont(int fontId)
{
  auto it = fonts.find(fontId);
  if (it == fonts.end())
  {
    std::cout << "Graphics: Invalid font ID: " << fontId << std::endl;
    return;
  }

  // Delete the atlas texture
  glDeleteTextures(1, &it->second.atlasTexture);

  fonts.erase(it);

  nextFontId--;

  std::cout << "Graphics: Unloaded font with ID " << fontId << std::endl;
}

// Unloads all fonts
void Graphics::unloadAllFonts()
{
  // Delete all atlas textures
  for (auto& pair : fonts)
  {
    glDeleteTextures(1, &pair.second.atlasTexture);
  }
  fonts.clear();

  // Reset the counter
  nextFontId = 0;

  std::cout << "Graphics: Unloaded all fonts" << std::endl;
}

// Draws text on the screen.
void Graphics::drawText(int fontId, const std::string& text, float x, float y, float scale, const glm::vec4& color)
{
  auto it = fonts.find(fontId);
  if (it == fonts.end())
  {
    std::cout << "Graphics: Invalid font ID: " << fontId << std::endl;
    return;
  }

  const Font& font = it->second;

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
    auto charIt = font.Characters.find(c);
    if (charIt == font.Characters.end())
      continue;

    const Character& ch = charIt->second;

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

  // Store the texture info and return its ID
  Texture tex = { texture, width, height };
  int textureId = nextTextureId++;
  textures[textureId] = tex;

  return textureId;
}

// A convenience function to draw a full texture.
void Graphics::drawTexture(int imageId, float x, float y, float width, float height, float rotation, Origin origin, const glm::vec4& tintColor)
{
  auto it = textures.find(imageId);
  if (it == textures.end())
  {
    std::cout << "Invalid image ID: " << imageId << std::endl;
    return;
  }
  // Calls drawTextureRegion with the full dimensions of the source texture.
  drawTextureRegion(
    imageId,
    x, y, width, height,
    0, 0, static_cast<float>(it->second.width), static_cast<float>(it->second.height),
    rotation,
    origin,
    tintColor
  );
}

// Adds a region of a texture to the current drawing batch.
void Graphics::drawTextureRegion(int imageId, float destX, float destY, float destWidth, float destHeight, float srcX, float srcY, float srcWidth, float srcHeight, float rotation, Origin origin, const glm::vec4& tintColor)
{
  auto it = textures.find(imageId);
  if (it == textures.end())
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
  const auto& tex = it->second;

  // Find if the texture is already in one of the active slots for this batch.
  auto texSlotIt = std::find(textureSlots.begin() + 1, textureSlots.begin() + textureSlotIndex, tex.id);
  if (texSlotIt != textureSlots.begin() + textureSlotIndex)
  {
    // If found, get its index.
    textureIndex = static_cast<float>(std::distance(textureSlots.begin(), texSlotIt));
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

  int spriteSheetId = nextSpriteSheetId++;
  spriteSheets[spriteSheetId] = spriteSheet;
  return spriteSheetId;
}

// Draws an animated sprite from a sprite sheet.
void Graphics::drawAnimatedSprite(int spriteSheetId, float x, float y, float width, float height, float time, float rotation, Origin origin, const glm::vec4& tintColor)
{
  auto it = spriteSheets.find(spriteSheetId);
  if (it == spriteSheets.end())
  {
    std::cout << "Invalid sprite sheet ID: " << spriteSheetId << std::endl;
    return;
  }

  const SpriteSheet& spriteSheet = it->second;

  // Calculate current frame based on time and animation speed
  int currentFrame = static_cast<int>(time * spriteSheet.animationSpeed) % spriteSheet.totalFrames;

  // Get texture dimensions to calculate frames per row
  auto texIt = textures.find(spriteSheet.textureId);
  if (texIt == textures.end())
  {
    std::cout << "Invalid texture ID in sprite sheet: " << spriteSheet.textureId << std::endl;
    return;
  }
  const Texture& texture = texIt->second;

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
  auto it = textures.find(textureId);
  if (it == textures.end())
  {
    std::cout << "Graphics: Invalid texture ID: " << textureId << std::endl;
    return;
  }

  // Save the texture ID before erasing
  unsigned int glTextureId = it->second.id;

  // Delete the OpenGL texture object
  glDeleteTextures(1, &glTextureId);

  // Clear the texture from any active texture slots
  for (uint32_t i = 0; i < MaxTextureSlots; i++)
  {
    if (textureSlots[i] == glTextureId)
    {
      textureSlots[i] = 0;
    }
  }

  // Remove from the textures map
  textures.erase(it);

  nextTextureId--;

  std::cout << "Graphics: Unloaded texture with ID " << textureId << std::endl;
}

// Unloads all textures
void Graphics::unloadAllTextures()
{
  for (const auto& pair : textures)
  {
    glDeleteTextures(1, &pair.second.id);
  }

  textures.clear();

  // Clear all texture slots except the white texture (slot 0)
  for (uint32_t i = 1; i < MaxTextureSlots; i++)
  {
    textureSlots[i] = 0;
  }

  textureSlotIndex = 1; // Reset to 1 (slot 0 is reserved for white texture)

  // Reset the counter
  nextTextureId = 0;

  std::cout << "Graphics: Unloaded all textures" << std::endl;
}

// Unloads a sprite sheet by its ID
void Graphics::unloadSpriteSheet(int spriteSheetId)
{
  auto it = spriteSheets.find(spriteSheetId);
  if (it == spriteSheets.end())
  {
    std::cout << "Graphics: Invalid sprite sheet ID: " << spriteSheetId << std::endl;
    return;
  }

  // Note: We don't delete the underlying texture here because it's managed
  // by the textures map. The sprite sheet just references a texture ID.
  // If you want to delete the underlying texture as well, you would call:
  // unloadTexture(spriteSheets[spriteSheetId].textureId);

  spriteSheets.erase(it);

  // If this was the last allocated sprite sheet, decrement the counter
  nextSpriteSheetId--;

  std::cout << "Graphics: Unloaded sprite sheet with ID " << spriteSheetId << std::endl;
}

// Unloads all sprite sheets
void Graphics::unloadAllSpriteSheets()
{
  spriteSheets.clear();

  // Reset the counter
  nextSpriteSheetId = 0;
}

// Gets the dimensions of a texture by its ID
void Graphics::getTextureDimensions(int textureId, int& width, int& height) const
{
  auto it = textures.find(textureId);
  if (it == textures.end())
  {
    width = 0;
    height = 0;
    return;
  }

  width = it->second.width;
  height = it->second.height;
}

// Gets the OpenGL texture handle by texture ID
unsigned int Graphics::getTextureHandle(int textureId) const
{
  auto it = textures.find(textureId);
  if (it == textures.end())
  {
    return 0;
  }

  return it->second.id;
}
