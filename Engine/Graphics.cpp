#include <GL/glew.h>
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"


namespace {

  unsigned int quadVAO;

  Shader basicShader;
  glm::mat4 projection = glm::ortho(0.0f, 800.0f, 600.0f, 0.0f, -1.0f, 1.0f); // TODO: window size should not be hardcoded


  void setupRectangle()
  {
    float vertices[] = {
      // pos
      0.0f, 1.0f,
      1.0f, 0.0f,
      0.0f, 0.0f,

      0.0f, 1.0f,
      1.0f, 1.0f,
      1.0f, 0.0f };

    glGenVertexArrays(1, &quadVAO);
    unsigned int vbo;
    glGenBuffers(1, &vbo);

    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }
}


namespace Graphics
{
  void init() {
    basicShader.init("basic.vert", "basic.frag");
    setupRectangle();
  }

  void drawRect(float x, float y, float width, float height, glm::vec3 color)
  {
    basicShader.use();
    basicShader.setMatrix("projection", projection);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));

    basicShader.setMatrix("model", model);
    basicShader.setVec3("uColor", color);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
  }

  void exit() {
    basicShader.destroy();
    glDeleteVertexArrays(1, &quadVAO);
  }
}