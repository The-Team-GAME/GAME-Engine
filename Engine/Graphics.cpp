#include <GL/glew.h>
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Shader.h"
#include <filesystem>
#include "Graphics.h"

namespace
{
  // Default quad vertices positions
  constexpr glm::vec4 QuadVertexPositions[] = {
      { -0.5f, -0.5f, 0.0f, 1.0f },
      {  0.5f, -0.5f, 0.0f, 1.0f },
      {  0.5f,  0.5f, 0.0f, 1.0f },
      { -0.5f,  0.5f, 0.0f, 1.0f }
  };

  // Default texture coordinates
  constexpr glm::vec2 TextureCoords[] = {
      { 0.0f, 0.0f },
      { 1.0f, 0.0f },
      { 1.0f, 1.0f },
      { 0.0f, 1.0f }
  };
}

void Graphics::updateViewMatrix() {
  view = glm::mat4(1.0f);
  // Translate to camera position
  view = glm::translate(view, glm::vec3(camera.position.x, camera.position.y, 0.0f));
  // Translate to center of viewport for proper pivoting
  view = glm::translate(view, glm::vec3(viewportWidth / 2.0f, viewportHeight / 2.0f, 0.0f));
  // Rotate around center
  view = glm::rotate(view, glm::radians(camera.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
  // Scale (zoom)
  view = glm::scale(view, glm::vec3(camera.zoom, camera.zoom, 1.0f));
  // Translate back
  view = glm::translate(view, glm::vec3(-viewportWidth / 2.0f, -viewportHeight / 2.0f, 0.0f));
  view = glm::inverse(view);
  viewProjectionMatrix = projection * view;
}

void Graphics::init()
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  textureShader.init("Assets\\shaders\\texture.vert", "Assets\\shaders\\texture.frag");

  quadVertexBufferBase = new Vertex[MaxVertices];

  glGenVertexArrays(1, &quadVAO);
  glBindVertexArray(quadVAO);

  glGenBuffers(1, &quadVBO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * MaxVertices, nullptr, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Position));

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Color));

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, TexCoords));

  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, TexID));

  uint32_t indices[MaxIndices];
  uint32_t offset = 0;
  for (uint32_t i = 0; i < MaxIndices; i += 6)
  {
    indices[i + 0] = offset + 0;
    indices[i + 1] = offset + 1;
    indices[i + 2] = offset + 2;

    indices[i + 3] = offset + 2;
    indices[i + 4] = offset + 3;
    indices[i + 5] = offset + 0;

    offset += 4;
  }

  glGenBuffers(1, &quadIBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // Create a 1x1 white texture for drawing solid colors
  unsigned int whiteTexture;
  glGenTextures(1, &whiteTexture);
  glBindTexture(GL_TEXTURE_2D, whiteTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  uint32_t color = 0xffffffff;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color);

  textureSlots[0] = whiteTexture;
  for (uint32_t i = 1; i < MaxTextureSlots; i++)
    textureSlots[i] = 0;

  // Set initial viewport and projection
  setViewport(800, 600); // TODO: hardcoded
  updateViewMatrix();
}

void Graphics::beginFrame()
{
  updateViewMatrix();
  startBatch();
}

void Graphics::endFrame()
{
  flush();
}

void Graphics::startBatch()
{
  quadIndexCount = 0;
  quadVertexBufferPtr = quadVertexBufferBase;
  textureSlotIndex = 1;
}

void Graphics::nextBatch()
{
  flush();
  startBatch();
}

void Graphics::flush()
{
  if (quadIndexCount == 0)
    return;

  uint32_t dataSize = (uint32_t)((uint8_t*)quadVertexBufferPtr - (uint8_t*)quadVertexBufferBase);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, quadVertexBufferBase);

  textureShader.use();
  textureShader.setMatrix("u_ViewProjection", viewProjectionMatrix);

  int samplers[MaxTextureSlots];
  for (int i = 0; i < MaxTextureSlots; i++)
    samplers[i] = i;
  textureShader.setIntArray("u_Textures", MaxTextureSlots, samplers);

  for (uint32_t i = 0; i < textureSlotIndex; i++)
    glBindTextureUnit(i, textureSlots[i]);

  glBindVertexArray(quadVAO);
  glDrawElements(GL_TRIANGLES, quadIndexCount, GL_UNSIGNED_INT, nullptr);
}

// TODO: implement this into the engine once event system is in place
void Graphics::setViewport(int width, int height)
{
  viewportWidth = width;
  viewportHeight = height;

  // Update the viewport
  glViewport(0, 0, width, height);

  // Update the projection matrix
  projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
  updateViewMatrix();
}

void Graphics::exit()
{
  textureShader.destroy();

  glDeleteVertexArrays(1, &quadVAO);
  glDeleteBuffers(1, &quadVBO);
  glDeleteBuffers(1, &quadIBO);

  delete[] quadVertexBufferBase;

  for (const auto& texture : textureSlots)
  {
    if (texture != 0) glDeleteTextures(1, &texture);
  }
}

void Graphics::setCameraPosition(float x, float y)
{
  camera.position = { x, y };
  updateViewMatrix();
}

void Graphics::setCameraZoom(float zoom)
{
  camera.zoom = zoom;
  updateViewMatrix();
}

void Graphics::setCameraRotation(float degrees)
{
  camera.rotation = degrees;
  updateViewMatrix();
}

void Graphics::drawRect(const DrawRectParams& params) {
  if (quadIndexCount >= MaxIndices)
  {
    nextBatch();
  }

  // Calculate offset based on origin
  glm::vec2 offset = { 0.0f, 0.0f };
  if (params.origin == Origin::TopLeft) {
    offset = { params.width / 2.0f, -params.height / 2.0f };
  }
  else if (params.origin == Origin::TopRight) {
    offset = { -params.width / 2.0f, -params.height / 2.0f };
  }
  else if (params.origin == Origin::BottomLeft) {
    offset = { params.width / 2.0f, params.height / 2.0f };
  }
  else if (params.origin == Origin::BottomRight) {
    offset = { -params.width / 2.0f, params.height / 2.0f };
  } // Center: no offset

  glm::mat4 transform = glm::translate(glm::mat4(1.0f), { params.x + offset.x, params.y + offset.y, 0.0f })
    * glm::rotate(glm::mat4(1.0f), glm::radians(params.rotation), { 0.0f, 0.0f, 1.0f })
    * glm::scale(glm::mat4(1.0f), { params.width, params.height, 1.0f });

  for (size_t i = 0; i < 4; i++)
  {
    quadVertexBufferPtr->Position = transform * QuadVertexPositions[i];
    quadVertexBufferPtr->Color = params.color;
    quadVertexBufferPtr->TexCoords = TextureCoords[i];
    quadVertexBufferPtr->TexID = -1.0f; // Use -1 for solid color
    quadVertexBufferPtr++;
  }

  quadIndexCount += 6;
}

int Graphics::loadTexture(const char* filepath, MinifyingFilter min_filter)
{
  // std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
  // std::cout << "Attempting to load image: " << filepath << std::endl;

  int width, height, nrChannels;
  unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);

  if (!data)
  {
    std::cout << "Failed to load texture: " << filepath << std::endl;
    return -1;
  }

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  if (min_filter == MinifyingFilter::Linear) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  else if (min_filter == MinifyingFilter::Nearest) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }


  GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(data);

  Texture tex = { texture, width, height };
  textures.push_back(tex);

  return static_cast<int>(textures.size() - 1);
}

void Graphics::drawTexture(const DrawTextureParams& params)
{
  if (params.imageId < 0 || params.imageId >= static_cast<int>(textures.size()))
  {
    std::cout << "Invalid image ID: " << params.imageId << std::endl;
    return;
  }
  drawTextureRegion({
    params.imageId,
    params.x, params.y, params.width, params.height,
    0, 0, static_cast<float>(textures[params.imageId].width), static_cast<float>(textures[params.imageId].height),
    params.rotation,
    params.origin,
    params.tintColor,
    });
}

void Graphics::drawTextureRegion(const DrawTextureRegionParams& params)
{
  if (params.imageId < 0 || params.imageId >= static_cast<int>(textures.size()))
  {
    std::cout << "Invalid image ID: " << params.imageId << std::endl;
    return;
  }

  if (quadIndexCount >= MaxIndices || textureSlotIndex >= MaxTextureSlots)
  {
    nextBatch();
  }

  float textureIndex = 0.0f;
  const auto& tex = textures[params.imageId];

  for (uint32_t i = 1; i < textureSlotIndex; i++)
  {
    if (textureSlots[i] == tex.id)
    {
      textureIndex = (float)i;
      break;
    }
  }

  if (textureIndex == 0.0f)
  {
    textureIndex = (float)textureSlotIndex;
    textureSlots[textureSlotIndex] = tex.id;
    textureSlotIndex++;
  }

  // Calculate offset based on origin
  glm::vec2 offset = { 0.0f, 0.0f };
  if (params.origin == Origin::TopLeft) {
    offset = { params.destWidth / 2.0f, -params.destHeight / 2.0f };
  }
  else if (params.origin == Origin::TopRight) {
    offset = { -params.destWidth / 2.0f, -params.destHeight / 2.0f };
  }
  else if (params.origin == Origin::BottomLeft) {
    offset = { params.destWidth / 2.0f, params.destHeight / 2.0f };
  }
  else if (params.origin == Origin::BottomRight) {
    offset = { -params.destWidth / 2.0f, params.destHeight / 2.0f };
  } // Center: no offset

  glm::mat4 transform = glm::translate(glm::mat4(1.0f), { params.destX + offset.x, params.destY + offset.y, 0.0f })
    * glm::rotate(glm::mat4(1.0f), glm::radians(params.rotation), { 0.0f, 0.0f, 1.0f })
    * glm::scale(glm::mat4(1.0f), { params.destWidth, params.destHeight, 1.0f });

  glm::vec2 texCoords[] = {
      { params.srcX / tex.width, params.srcY / tex.height },
      { (params.srcX + params.srcWidth) / tex.width, params.srcY / tex.height },
      { (params.srcX + params.srcWidth) / tex.width, (params.srcY + params.srcHeight) / tex.height },
      { params.srcX / tex.width, (params.srcY + params.srcHeight) / tex.height }
  };

  for (size_t i = 0; i < 4; i++)
  {
    quadVertexBufferPtr->Position = transform * QuadVertexPositions[i];
    quadVertexBufferPtr->Color = params.tintColor;
    quadVertexBufferPtr->TexCoords = texCoords[i];
    quadVertexBufferPtr->TexID = textureIndex;
    quadVertexBufferPtr++;
  }

  quadIndexCount += 6;
}

void Graphics::clearScreen(float r, float g, float b, float a)
{
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);
}
