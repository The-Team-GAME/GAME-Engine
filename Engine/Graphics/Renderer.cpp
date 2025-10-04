#include "Renderer.h"
#include "../Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace Engine {
  namespace Graphics {

    // Shader program
    static GLuint shader_program = 0;
    static GLint u_projection = -1;
    static GLint u_view = -1;
    static GLint u_texture = -1;

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

    // Compile shader helper function
    static GLuint compile_shader(GLenum type, const char* source) {
      GLuint shader = glCreateShader(type);
      glShaderSource(shader, 1, &source, nullptr);
      glCompileShader(shader);

      GLint success;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        Log::crash(std::string("Shader compilation failed: ") + info_log);
        return 0;
      }

      return shader;
    }

    // Create shader program
    static GLuint create_shader_program() {
      // Vertex shader
      const char* vertex_shader_source = R"(
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

      // Fragment shader
      const char* fragment_shader_source = R"(
        #version 330 core
        in vec2 v_texCoord;
        in vec4 v_color;
        
        out vec4 FragColor;
        
        uniform sampler2D u_texture;
        
        void main() {
          FragColor = texture(u_texture, v_texCoord) * v_color;
        }
      )";

      GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
      GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

      if (vertex_shader == 0 || fragment_shader == 0) {
        return 0;
      }

      GLuint program = glCreateProgram();
      glAttachShader(program, vertex_shader);
      glAttachShader(program, fragment_shader);
      glLinkProgram(program);

      GLint success;
      glGetProgramiv(program, GL_LINK_STATUS, &success);
      if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program, 512, nullptr, info_log);
        Log::crash(std::string("Shader program linking failed: ") + info_log);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return 0;
      }

      glDeleteShader(vertex_shader);
      glDeleteShader(fragment_shader);

      return program;
    }

    bool init() {
      // Create shader program
      shader_program = create_shader_program();
      if (shader_program == 0) {
        Log::crash("Failed to create shader program");
        return false;
      }

      // Get uniform locations
      u_projection = glGetUniformLocation(shader_program, "u_projection");
      u_view = glGetUniformLocation(shader_program, "u_view");
      u_texture = glGetUniformLocation(shader_program, "u_texture");

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

      if (shader_program != 0) {
        glDeleteProgram(shader_program);
        shader_program = 0;
      }

      Log::info("Renderer cleaned up");
      return true;
    }

    void draw_texture(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color) {
      if (!texture || texture->id == 0) {
        Log::warning("Attempted to draw invalid texture");
        return;
      }

      // Use shader program
      glUseProgram(shader_program);

      // Bind texture
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture->id);
      glUniform1i(u_texture, 0);

      // Create transformation matrix
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(position.x, position.y, 0.0f));

      if (rotation != 0.0f) {
        model = glm::translate(model, glm::vec3(size.x * 0.5f, size.y * 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-size.x * 0.5f, -size.y * 0.5f, 0.0f));
      }

      model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

      // For now, use identity view matrix (this should be set by camera system)
      glm::mat4 view = glm::mat4(1.0f);
      glUniformMatrix4fv(u_view, 1, GL_FALSE, &view[0][0]);

      // Set projection matrix (orthographic projection)
      // This should ideally be set once per frame, not per draw call
      // For now, we'll use a simple orthographic projection
      glm::mat4 projection = glm::ortho(0.0f, 1920.0f, 1080.0f, 0.0f, -1.0f, 1.0f);
      glUniformMatrix4fv(u_projection, 1, GL_FALSE, &projection[0][0]);

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
