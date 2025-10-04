#include "Renderer.h"
#include "Shader.h"
#include "../Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

namespace Engine {
  namespace Graphics {

    // Shader program
    static std::unique_ptr<Shader> shader;

    // Vertex Array Object and Vertex Buffer Object
    static GLuint vao = 0;
    static GLuint vbo = 0;
    static GLuint ebo = 0;

    // Vertex structure for a textured quad
    struct Vertex {
      glm::vec2 position;
      glm::vec2 texCoord;
      glm::vec4 color;
    };

    bool init() {
      // Vertex shader source
      const std::string vertex_shader_source = R"(
        #version 330 core
        layout (location = 0) in vec2 a_position;
        layout (location = 1) in vec2 a_texCoord;
        layout (location = 2) in vec4 a_color;
        
        out vec2 v_texCoord;
        out vec4 v_color;
        
        uniform mat4 u_projection;
        uniform mat4 u_view;
        
        void main() {
          gl_Position = u_projection * u_view * vec4(a_position, 0.0, 1.0);
          v_texCoord = a_texCoord;
          v_color = a_color;
        }
      )";

      // Fragment shader source
      const std::string fragment_shader_source = R"(
        #version 330 core
        in vec2 v_texCoord;
        in vec4 v_color;
        
        out vec4 FragColor;
        
        uniform sampler2D u_texture;
        
        void main() {
          FragColor = texture(u_texture, v_texCoord) * v_color;
        }
      )";

      // Create shader
      shader = std::make_unique<Shader>(vertex_shader_source, fragment_shader_source);

      if (!shader->is_valid()) {
        Log::error("Failed to create shader");
        return false;
      }

      // Create VAO and VBO for a quad
      glGenVertexArrays(1, &vao);
      glGenBuffers(1, &vbo);
      glGenBuffers(1, &ebo);

      glBindVertexArray(vao);

      // Allocate buffer for 4 vertices (quad)
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4, nullptr, GL_DYNAMIC_DRAW);

      // Create index buffer for quad (2 triangles)
      GLuint indices[] = { 0, 1, 2, 2, 3, 0 };
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

      // Position attribute
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

      // TexCoord attribute
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

      // Color attribute
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

      glBindVertexArray(0);

      // Setup OpenGL state
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      Log::info("Renderer initialized");
      return true;
    }

    bool exit() {
      // Delete OpenGL resources
      if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
      }

      if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
      }

      if (ebo != 0) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
      }

      // Delete shader
      shader.reset();

      Log::info("Renderer cleaned up");
      return true;
    }

    void draw_texture(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color) {
      if (!texture || texture->id == 0) {
        Log::warning("Attempted to draw invalid texture");
        return;
      }

      if (!shader || !shader->is_valid()) {
        Log::error("Shader is not valid");
        return;
      }

      // Use shader
      shader->use();

      // Bind texture
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture->id);
      shader->set_int("u_texture", 0);

      // Create transformation matrix
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(position.x, position.y, 0.0f));

      if (rotation != 0.0f) {
        model = glm::translate(model, glm::vec3(size.x * 0.5f, size.y * 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-size.x * 0.5f, -size.y * 0.5f, 0.0f));
      }

      model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

      // Set view matrix (identity for now)
      glm::mat4 view = glm::mat4(1.0f);
      shader->set_mat4("u_view", view);

      // Set projection matrix
      glm::mat4 projection = glm::ortho(0.0f, 1920.0f, 1080.0f, 0.0f, -1.0f, 1.0f);
      shader->set_mat4("u_projection", projection);

      // Create quad vertices
      Vertex vertices[4];

      // Convert color from 0-255 to 0-1
      glm::vec4 normalized_color(color.x / 255.0f, color.y / 255.0f, color.z / 255.0f, color.w / 255.0f);

      // Bottom-left
      vertices[0].position = glm::vec2(0.0f, 1.0f);
      vertices[0].texCoord = glm::vec2(0.0f, 1.0f);
      vertices[0].color = normalized_color;

      // Bottom-right
      vertices[1].position = glm::vec2(1.0f, 1.0f);
      vertices[1].texCoord = glm::vec2(1.0f, 1.0f);
      vertices[1].color = normalized_color;

      // Top-right
      vertices[2].position = glm::vec2(1.0f, 0.0f);
      vertices[2].texCoord = glm::vec2(1.0f, 0.0f);
      vertices[2].color = normalized_color;

      // Top-left
      vertices[3].position = glm::vec2(0.0f, 0.0f);
      vertices[3].texCoord = glm::vec2(0.0f, 0.0f);
      vertices[3].color = normalized_color;

      // Transform vertices by model matrix
      for (int i = 0; i < 4; i++) {
        glm::vec4 transformed = model * glm::vec4(vertices[i].position, 0.0f, 1.0f);
        vertices[i].position = glm::vec2(transformed.x, transformed.y);
      }

      // Upload vertices to GPU
      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

      // Draw quad
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

      glBindVertexArray(0);
    }

  } // namespace Graphics
} // namespace Engine
