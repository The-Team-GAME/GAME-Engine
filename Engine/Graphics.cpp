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

void Graphics::setupRectangle()
{
  float vertices[] = {
    // pos      // tex coords
    0.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,

    0.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 0.0f };

  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);  // Store VBO

  glBindVertexArray(quadVAO);

  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);  // Use stored VBO
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Position attribute
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

  // Texture coordinate attribute
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
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
}

void Graphics::init()
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // basicShader.init("Assets\\shaders\\basic.vert", "Assets\\shaders\\basic.frag");
  textureShader.init("Assets\\shaders\\texture.vert", "Assets\\shaders\\texture.frag");
  setupRectangle();

  updateViewMatrix();

  // Set initial viewport and projection
  setViewport(800, 600); // TODO: hardcoded
}

void Graphics::beginFrame()
{
  // This is the ideal place to update shader uniforms that change per-frame, like the view matrix.
  textureShader.use();
  textureShader.setMatrix("view", view);
  glUseProgram(0);
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

  // Update the projection matrix in the shader
  textureShader.use();
  textureShader.setMatrix("projection", projection);
  glUseProgram(0); // Unbind shader program
}

void Graphics::exit()
{
  // basicShader.destroy();
  textureShader.destroy();

  for (const auto& texture : textures)
  {
    glDeleteTextures(1, &texture.id);
  }
  textures.clear();

  if (quadVAO != 0) glDeleteVertexArrays(1, &quadVAO);
  if (quadVBO != 0) glDeleteBuffers(1, &quadVBO);
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

void Graphics::drawRect(float x, float y, float width, float height, glm::vec4 color, float rotation) {
  textureShader.use();
  glm::mat4 model = glm::mat4(1.0f);
  applyModelTransform(model, x, y, width, height, rotation);  // Now supports rotation
  textureShader.setMatrix("model", model);
  textureShader.setVec4("uColor", color);
  textureShader.setBool("useTexture", false);
  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
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


void Graphics::applyModelTransform(glm::mat4& model, float x, float y, float width, float height, float rotation)
{
  model = glm::translate(model, glm::vec3(x, y, 0.0f));
  model = glm::translate(model, glm::vec3(0.5f * width, 0.5f * height, 0.0f));
  model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::translate(model, glm::vec3(-0.5f * width, -0.5f * height, 0.0f));
  model = glm::scale(model, glm::vec3(width, height, 1.0f));
}

void Graphics::drawTexture(int imageId, float x, float y, float width, float height, float rotation)
{
  if (imageId < 0 || imageId >= static_cast<int>(textures.size()))
  {
    std::cout << "Invalid image ID: " << imageId << std::endl;
    return;
  }

  textureShader.use();

  glm::mat4 model = glm::mat4(1.0f);
  applyModelTransform(model, x, y, width, height, rotation);  // Use helper

  textureShader.setMatrix("model", model);
  textureShader.setBool("useTexture", true);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textures[imageId].id);

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}

void Graphics::drawTextureRegion(int imageId, float destX, float destY, float destWidth, float destHeight, float srcX, float srcY, float srcWidth, float srcHeight, float rotation)
{
  if (imageId < 0 || imageId >= static_cast<int>(textures.size()))
  {
    std::cout << "Invalid image ID: " << imageId << std::endl;
    return;
  }

  textureShader.use();

  glm::mat4 model = glm::mat4(1.0f);
  applyModelTransform(model, destX, destY, destWidth, destHeight, rotation);  // Use helper

  textureShader.setMatrix("model", model);
  textureShader.setBool("useTexture", true);

  const auto& tex = textures[imageId];
  glm::vec4 spriteCoords(srcX / tex.width, srcY / tex.height, srcWidth / tex.width, srcHeight / tex.height);
  textureShader.setVec4("spriteCoords", spriteCoords);
  textureShader.setBool("useTextureRegion", true);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex.id);

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);

  textureShader.setBool("useTextureRegion", false);
}

void Graphics::clearScreen(float r, float g, float b, float a)
{
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);
}
