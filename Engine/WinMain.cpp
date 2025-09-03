#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


unsigned int quadVAO;

void setup_rectangle()
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

void drawRect(Shader& shader, float x, float y, float width, float height, glm::vec3 color)
{
  shader.use();
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(x, y, 0.0f));
  model = glm::scale(model, glm::vec3(width, height, 1.0f));

  shader.setMatrix("model", model);
  shader.setVec3("uColor", color);

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}

int main(void)
{
  GLFWwindow* window;

  /* Initialize the library */
  if (!glfwInit())
    return -1;

  /* Create a windowed mode window and its OpenGL context */
  int windowWidth = 800;
  int windowHeight = 600;
  window = glfwCreateWindow(windowWidth, windowHeight, "Rectangle Drawer", NULL, NULL);
  if (!window)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  /* Make the window's context current */
  glfwMakeContextCurrent(window);

  if (glewInit() != GLEW_OK)
  {
    std::cout << "GLEW Initialization failed.\n";
  }

  std::cout << glGetString(GL_VERSION) << std::endl;

  setup_rectangle();

  Shader basicShader("basic.vert", "basic.frag");

  glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, (float)windowHeight, 0.0f, -1.0f, 1.0f);
  basicShader.use();
  basicShader.setMatrix("projection", projection);

  /* Loop until the user closes the window */
  while (!glfwWindowShouldClose(window))
  {
    /* Render here */
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw rectangles of different sizes and colors
    drawRect(basicShader, 100.0f, 100.0f, 200.0f, 50.0f, glm::vec3(1.0f, 0.0f, 0.0f));  // Red
    drawRect(basicShader, 350.0f, 200.0f, 100.0f, 150.0f, glm::vec3(0.0f, 1.0f, 0.0f)); // Green
    drawRect(basicShader, 50.0f, 400.0f, 75.0f, 75.0f, glm::vec3(0.0f, 0.0f, 1.0f));    // Blue

    /* Swap front and back buffers */
    glfwSwapBuffers(window);

    /* Poll for and process events */
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}