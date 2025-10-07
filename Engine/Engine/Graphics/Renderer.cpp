#include "Renderer.h"
#include "Shader.h"
#include "../Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include "../Resources.h"

namespace Engine {
  namespace Graphics {

    // Shader program
    static std::unique_ptr<Shader> shader;

    // Vertex Array Object and Vertex Buffer Object
    static GLuint vao = 0;
    static GLuint vbo = 0;
    static GLuint ebo = 0;

    // Virtual resolution
    static float virtual_width = 1600.0f;
    static float virtual_height = 900.0f;

    // Camera state
    static Camera current_camera;

    // Vertex structure for a textured quad
    struct Vertex {
      glm::vec2 position;
      glm::vec2 texCoord;
      glm::vec4 color;
    };

    // Helper function to get the current projection matrix
    static glm::mat4 get_projection_matrix() {
      return glm::ortho(0.0f, virtual_width, virtual_height, 0.0f, -1.0f, 1.0f);
    }

    // Helper function to get the current view matrix based on camera
    static glm::mat4 get_view_matrix() {
      glm::mat4 view = glm::mat4(1.0f);

      // Calculate the center of the screen in virtual coordinates
      float center_x = virtual_width * 0.5f;
      float center_y = virtual_height * 0.5f;

      // Step 1: Translate to camera position (inverted for camera movement)
      view = glm::translate(view, glm::vec3(-current_camera.position.x, -current_camera.position.y, 0.0f));

      // Step 2: Translate to center (for rotation and zoom around center)
      view = glm::translate(view, glm::vec3(center_x, center_y, 0.0f));

      // Step 3: Apply rotation around center
      if (current_camera.rotation != 0.0f) {
        view = glm::rotate(view, glm::radians(-current_camera.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      }

      // Step 4: Apply zoom (scale) around center
      view = glm::scale(view, glm::vec3(current_camera.zoom, current_camera.zoom, 1.0f));

      // Step 5: Translate back from center
      view = glm::translate(view, glm::vec3(-center_x, -center_y, 0.0f));

      return view;
    }

    // Helper function to draw a quad with custom texture coordinates
    static void draw_quad(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size,
      f32 rotation, const Math::Vec4D& color, const std::array<glm::vec2, 4>& tex_coords) {
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
      // Position is now treated as the center of the object
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(position.x, position.y, 0.0f));

      if (rotation != 0.0f) {
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      }

      model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

      // Set view matrix (using camera)
      shader->set_mat4("u_view", get_view_matrix());

      // Set projection matrix using virtual resolution
      shader->set_mat4("u_projection", get_projection_matrix());

      // Create quad vertices
      Vertex vertices[4];

      // Convert color from 0-255 to 0-1
      glm::vec4 normalized_color(color.x, color.y, color.z, color.w);

      // Vertices are now centered around the origin (0, 0)
      // Bottom-left
      vertices[0].position = glm::vec2(-0.5f, 0.5f);
      vertices[0].texCoord = tex_coords[0];
      vertices[0].color = normalized_color;

      // Bottom-right
      vertices[1].position = glm::vec2(0.5f, 0.5f);
      vertices[1].texCoord = tex_coords[1];
      vertices[1].color = normalized_color;

      // Top-right
      vertices[2].position = glm::vec2(0.5f, -0.5f);
      vertices[2].texCoord = tex_coords[2];
      vertices[2].color = normalized_color;

      // Top-left
      vertices[3].position = glm::vec2(-0.5f, -0.5f);
      vertices[3].texCoord = tex_coords[3];
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

    // Helper function to draw a glyph (character) at a specific position without rotation
    static void draw_glyph(const TextureData& atlas_texture, const std::array<glm::vec2, 4>& tex_coords,
      const Math::Vec2D& position, const Math::Vec2D& size, const Math::Vec4D& color) {
      if (atlas_texture.id == 0) {
        return;
      }

      if (!shader || !shader->is_valid()) {
        return;
      }

      // Use shader
      shader->use();

      // Bind texture atlas
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, atlas_texture.id);
      shader->set_int("u_texture", 0);

      // Set view matrix (using camera)
      shader->set_mat4("u_view", get_view_matrix());

      // Set projection matrix using virtual resolution
      shader->set_mat4("u_projection", get_projection_matrix());

      // Create quad vertices (no rotation for text glyphs)
      Vertex vertices[4];

      // Convert color from 0-255 to 0-1
      glm::vec4 normalized_color(color.x, color.y, color.z, color.w);

      // Bottom-left
      vertices[0].position = glm::vec2(position.x, position.y + size.y);
      vertices[0].texCoord = tex_coords[0];
      vertices[0].color = normalized_color;

      // Bottom-right
      vertices[1].position = glm::vec2(position.x + size.x, position.y + size.y);
      vertices[1].texCoord = tex_coords[1];
      vertices[1].color = normalized_color;

      // Top-right
      vertices[2].position = glm::vec2(position.x + size.x, position.y);
      vertices[2].texCoord = tex_coords[2];
      vertices[2].color = normalized_color;

      // Top-left
      vertices[3].position = glm::vec2(position.x, position.y);
      vertices[3].texCoord = tex_coords[3];
      vertices[3].color = normalized_color;

      // Upload vertices to GPU
      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

      // Draw quad
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

      glBindVertexArray(0);
    }

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

      // Initialize camera to default
      reset_camera();

      Log::info("Renderer initialized with virtual resolution: " +
        std::to_string((int)virtual_width) + "x" + std::to_string((int)virtual_height));
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

    void set_virtual_resolution(f32 width, f32 height) {
      virtual_width = width;
      virtual_height = height;
      Log::info("Virtual resolution set to: " +
        std::to_string((int)width) + "x" + std::to_string((int)height));
    }

    Math::Vec2D get_virtual_resolution() {
      return Math::Vec2D(virtual_width, virtual_height);
    }

    void set_camera(const Camera& camera) {
      current_camera = camera;
    }

    Camera get_camera() {
      return current_camera;
    }

    void set_camera_position(const Math::Vec2D& position) {
      current_camera.position = position;
    }

    void set_camera_rotation(f32 rotation) {
      current_camera.rotation = rotation;
    }

    void set_camera_zoom(f32 zoom) {
      current_camera.zoom = zoom;
    }

    void reset_camera() {
      current_camera.position = Math::Vec2D(0.0f, 0.0f);
      current_camera.rotation = 0.0f;
      current_camera.zoom = 1.0f;
    }

    void draw_rect(const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color)
    {
      draw_texture(Engine::Resources::get_unit_rect_texture(), position, size, rotation, color);
    }

    void draw_texture(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color) {
      // Default texture coordinates for full texture (bottom-left, bottom-right, top-right, top-left)
      std::array<glm::vec2, 4> default_tex_coords = {
        glm::vec2(0.0f, 1.0f), // Bottom-left
        glm::vec2(1.0f, 1.0f), // Bottom-right
        glm::vec2(1.0f, 0.0f), // Top-right
        glm::vec2(0.0f, 0.0f)  // Top-left
      };

      draw_quad(texture, position, size, rotation, color, default_tex_coords);
    }

    void draw_subtexture(Engine::Graphics::Texture texture, const std::array<glm::vec2, 4>& tex_coords,
      const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color) {
      draw_quad(texture, position, size, rotation, color, tex_coords);
    }

    void draw_text(Engine::Graphics::Font font, const std::string& text, const Math::Vec2D& position, f32 scale, const Math::Vec4D& color) {
      if (!font || font->texture_atlas.id == 0) {
        Log::warning("Attempted to draw text with invalid font");
        return;
      }

      if (text.empty()) {
        return;
      }

      // Starting position for text rendering
      float x = position.x;
      float y = position.y;

      // Iterate through all characters in the text
      for (char c : text) {
        // Get the glyph for this character
        auto it = font->glyphs.find(static_cast<unsigned int>(c));

        if (it == font->glyphs.end()) {
          // Character not found in font, skip it
          continue;
        }

        const Glyph& glyph = it->second;

        // Calculate position and size for this glyph
        float xpos = x + glyph.bearing.x * scale;
        float ypos = y + (font->baseline - glyph.bearing.y) * scale;

        float w = glyph.size.x * scale;
        float h = glyph.size.y * scale;

        // Draw the glyph
        draw_glyph(font->texture_atlas, glyph.tex_coords,
          Math::Vec2D(xpos, ypos), Math::Vec2D(w, h), color);

        // Advance cursor to next glyph position
        // Note: advance is in 1/64th pixels, so we shift by 6 to get pixels
        x += (glyph.advance >> 6) * scale;
      }
    }

    void clear_screen(const Math::Vec4D& color) {
      glClearColor(color.x, color.y, color.z, color.w);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void clear_screen(Engine::Graphics::Texture texture) {
      // Clear with black first
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // Draw the texture to fill the entire screen
      Math::Vec2D resolution = get_virtual_resolution();
      draw_texture(
        texture,
        Math::Vec2D(resolution.x * 0.5f, resolution.y * 0.5f), // Center position
        resolution, // Full screen size
        0.0f, // No rotation
        Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f) // White (no tint)
      );
    }

  } // namespace Graphics
} // namespace Engine
