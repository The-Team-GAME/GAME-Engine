#include <GL/glew.h>
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <numeric>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
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
  view = glm::mat4(1.0f); // Start with an identity matrix
  // The transformations are applied in reverse order of how they appear.
  // We want to move the world, not the camera, so we apply the inverse of the camera's transform.
  // 1. Translate to the camera's position.
  view = glm::translate(view, glm::vec3(camera.position.x, camera.position.y, 0.0f));
  // 2. Translate to the center of the screen to rotate around the center.
  view = glm::translate(view, glm::vec3(viewportWidth / 2.0f, viewportHeight / 2.0f, 0.0f));
  // 3. Apply rotation.
  view = glm::rotate(view, glm::radians(camera.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
  // 4. Apply zoom (scaling).
  view = glm::scale(view, glm::vec3(camera.zoom, camera.zoom, 1.0f));
  // 5. Translate back from the center.
  view = glm::translate(view, glm::vec3(-viewportWidth / 2.0f, -viewportHeight / 2.0f, 0.0f));
  // 6. Invert the matrix to get the final view matrix.
  view = glm::inverse(view);
  // Pre-calculate the combined view-projection matrix for the shader.
  viewProjectionMatrix = projection * view;
}

// Initializes the graphics system, shaders, and buffers.
void Graphics::init(int windowWidth, int windowHeight)
{
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
  updateViewMatrix();
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
  projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
  updateViewMatrix(); // Recalculate the combined matrix.
}

// Cleans up OpenGL objects and resources.
void Graphics::exit()
{
  textureShader.destroy();

  glDeleteVertexArrays(1, &quadVAO);
  glDeleteBuffers(1, &quadVBO);
  glDeleteBuffers(1, &quadIBO);

  // Delete all loaded textures to prevent memory leaks.
  for (const auto& tex : textures)
  {
    glDeleteTextures(1, &tex.id);
  }

  // Delete the white texture and any others that might be in the slots.
  for (const auto& texture : textureSlots)
  {
    if (texture != 0) glDeleteTextures(1, &texture);
  }
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

// Clears the screen with a specified color.
void Graphics::clearScreen(float r, float g, float b, float a)
{
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);
}
