#include "Renderer.h"
#include "Shader.h"
#include "../Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include "../Resources.h"
#include <vector>
#include "../Perf.h"
#include "../ECS/ECS.h"
#include "../Events/WindowEvents.h"

namespace Engine {
  namespace Graphics {

    // Shader program
    std::unique_ptr<Shader> shader;

    // Vertex Array Object and Vertex Buffer Object
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    // Virtual resolution
    float virtual_width = 1600.0f;
    float virtual_height = 900.0f;

    // Camera state
    Camera current_camera;

    // Batch rendering constants
    constexpr size_t MAX_QUAD_COUNT = 10000;
    constexpr size_t MAX_VERTEX_COUNT = MAX_QUAD_COUNT * 4;
    constexpr size_t MAX_INDEX_COUNT = MAX_QUAD_COUNT * 6;

    // Vertex structure for a textured quad
    struct Vertex {
      glm::vec2 position;
      glm::vec2 texCoord;
      glm::vec4 color;
      glm::vec2 parallax_factor;
    };

    // Batch state
    struct BatchState {
      std::vector<Vertex> vertices;
      GLuint current_texture_id = 0;
      size_t quad_count = 0;

      void reset() {
        vertices.clear();
        current_texture_id = 0;
        quad_count = 0;
      }
    };

    BatchState batch_state;

    Viewport current_viewport;

    // Helper function to get the current projection matrix
    glm::mat4 get_projection_matrix() {
      return glm::ortho(0.0f, virtual_width, virtual_height, 0.0f, -1.0f, 1.0f);
    }

    // Helper function to get the current view matrix based on camera
    glm::mat4 get_view_matrix() {
      glm::mat4 view = glm::mat4(1.0f);

      // Calculate the center of the screen in virtual coordinates
      float center_x = virtual_width * 0.5f;
      float center_y = virtual_height * 0.5f;

      // Step 1: Translate to center (for rotation and zoom around center)
      view = glm::translate(view, glm::vec3(center_x, center_y, 0.0f));

      // Step 2: Apply rotation around center
      if (current_camera.rotation != 0.0f) {
        view = glm::rotate(view, glm::radians(-current_camera.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      }

      // Step 3: Apply zoom (scale) around center
      view = glm::scale(view, glm::vec3(current_camera.zoom, current_camera.zoom, 1.0f));

      // Step 4: Translate back from center
      view = glm::translate(view, glm::vec3(-center_x, -center_y, 0.0f));

      // Step 5: Translate to camera position (inverted for camera movement)
      view = glm::translate(view, glm::vec3(-current_camera.position.x, -current_camera.position.y, 0.0f));

      return view;
    }

    // Flush the current batch to GPU
    void flush_batch() {
      if (batch_state.quad_count == 0) {
        return;
      }

      // Use shader
      shader->use();

      // Bind texture
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, batch_state.current_texture_id);
      shader->set_int("u_texture", 0);

      // Set view and projection matrices
      shader->set_mat4("u_view", get_view_matrix());
      shader->set_mat4("u_projection", get_projection_matrix());

      // Set camera position for parallax calculation on GPU
      shader->set_vec2("u_camera_position", current_camera.position.x, current_camera.position.y);

      // Upload vertices to GPU
      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, batch_state.vertices.size() * sizeof(Vertex), batch_state.vertices.data());

      // Draw all quads in one call
      glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batch_state.quad_count * 6), GL_UNSIGNED_INT, 0);

      glBindVertexArray(0);

      // Reset batch
      batch_state.reset();
    }

    // Begin a new batch (call at start of frame)
    void begin_batch() {
      batch_state.reset();
    }

    // End batch (call at end of frame)
    void end_batch() {
      Engine::Perf::start("Graphics flush");
      flush_batch();
      Engine::Perf::stop("Graphics flush");
    }

    // Add a quad to the batch
    void batch_quad(GLuint texture_id, const Vertex vertices[4]) {
      // Check if we need to flush due to texture change
      if (batch_state.current_texture_id != 0 && batch_state.current_texture_id != texture_id) {
        flush_batch();
      }

      // Check if we need to flush due to batch being full
      if (batch_state.quad_count >= MAX_QUAD_COUNT) {
        flush_batch();
      }

      // Set current texture
      batch_state.current_texture_id = texture_id;

      // Add vertices to batch
      for (int i = 0; i < 4; i++) {
        batch_state.vertices.push_back(vertices[i]);
      }

      batch_state.quad_count++;
    }

    // Helper function to draw a quad with custom texture coordinates
    void draw_quad(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size,
      f32 rotation, const Math::Vec4D& color, const std::array<Math::Vec2D, 4>& tex_coords,
      const Math::Vec2D& parallax_factor = Math::Vec2D(1.0f, 1.0f)) {
      if (!texture || texture->id == 0) {
        Log::warning("Attempted to draw invalid texture");
        return;
      }

      if (!shader || !shader->is_valid()) {
        Log::error("Shader is not valid");
        return;
      }

      // Create transformation matrix
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(position.x, position.y, 0.0f));

      if (rotation != 0.0f) {
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      }

      model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

      // Create quad vertices
      Vertex vertices[4];

      // Convert color from 0-1 range
      glm::vec4 normalized_color(color.x, color.y, color.z, color.w);

      // Vertices are centered around the origin
      glm::vec2 base_positions[4] = {
        glm::vec2(-0.5f, 0.5f),  // Bottom-left
        glm::vec2(0.5f, 0.5f),   // Bottom-right
        glm::vec2(0.5f, -0.5f),  // Top-right
        glm::vec2(-0.5f, -0.5f)  // Top-left
      };

      // Transform vertices by model matrix
      for (int i = 0; i < 4; i++) {
        glm::vec4 transformed = model * glm::vec4(base_positions[i], 0.0f, 1.0f);
        vertices[i].position = glm::vec2(transformed.x, transformed.y);
        vertices[i].texCoord = glm::vec2(tex_coords[i].x, tex_coords[i].y);
        vertices[i].color = normalized_color;
        vertices[i].parallax_factor = glm::vec2(parallax_factor.x, parallax_factor.y);
      }

      // Add to batch instead of drawing immediately
      batch_quad(texture->id, vertices);
    }

    // Helper function to draw a glyph (character) at a specific position without rotation
    void draw_glyph(const TextureData& atlas_texture, const std::array<Math::Vec2D, 4>& tex_coords,
      const Math::Vec2D& position, const Math::Vec2D& size, const Math::Vec4D& color, const Math::Vec2D& parallax_factor = Math::Vec2D(1.0f, 1.0f)) {
      if (atlas_texture.id == 0) {
        return;
      }

      if (!shader || !shader->is_valid()) {
        return;
      }

      // Create quad vertices (no rotation for text glyphs)
      Vertex vertices[4];

      // Convert color from 0-1 range
      glm::vec4 normalized_color(color.x, color.y, color.z, color.w);

      // Bottom-left
      vertices[0].position = glm::vec2(position.x, position.y + size.y);
      vertices[0].texCoord = glm::vec2(tex_coords[0].x, tex_coords[0].y);
      vertices[0].color = normalized_color;
      vertices[0].parallax_factor = glm::vec2(parallax_factor.x, parallax_factor.y);

      // Bottom-right
      vertices[1].position = glm::vec2(position.x + size.x, position.y + size.y);
      vertices[1].texCoord = glm::vec2(tex_coords[1].x, tex_coords[1].y);
      vertices[1].color = normalized_color;
      vertices[1].parallax_factor = glm::vec2(parallax_factor.x, parallax_factor.y);

      // Top-right
      vertices[2].position = glm::vec2(position.x + size.x, position.y);
      vertices[2].texCoord = glm::vec2(tex_coords[2].x, tex_coords[2].y);
      vertices[2].color = normalized_color;
      vertices[2].parallax_factor = glm::vec2(parallax_factor.x, parallax_factor.y);

      // Top-left
      vertices[3].position = glm::vec2(position.x, position.y);
      vertices[3].texCoord = glm::vec2(tex_coords[3].x, tex_coords[3].y);
      vertices[3].color = normalized_color;
      vertices[3].parallax_factor = glm::vec2(parallax_factor.x, parallax_factor.y);

      // Add to batch instead of drawing immediately
      batch_quad(atlas_texture.id, vertices);
    }

    bool init() {
      // Vertex shader source
      const std::string vertex_shader_source = R"(
        #version 330 core
        layout (location = 0) in vec2 a_position;
        layout (location = 1) in vec2 a_texCoord;
        layout (location = 2) in vec4 a_color;
        layout (location = 3) in vec2 a_parallax_factor;
        
        out vec2 v_texCoord;
        out vec4 v_color;
        
        uniform mat4 u_projection;
        uniform mat4 u_view;
        uniform vec2 u_camera_position;
        
        void main() {
          vec2 adjusted_position = a_position + u_camera_position * (1.0 - a_parallax_factor);
          gl_Position = u_projection * u_view * vec4(adjusted_position, 0.0, 1.0);
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

      // Create VAO and VBO for batched quads
      glGenVertexArrays(1, &vao);
      glGenBuffers(1, &vbo);
      glGenBuffers(1, &ebo);

      glBindVertexArray(vao);

      // Allocate buffer for maximum number of vertices
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * MAX_VERTEX_COUNT, nullptr, GL_DYNAMIC_DRAW);

      // Create index buffer for maximum number of quads
      std::vector<GLuint> indices;
      indices.reserve(MAX_INDEX_COUNT);
      for (size_t i = 0; i < MAX_QUAD_COUNT; i++) {
        GLuint offset = static_cast<GLuint>(i * 4);
        indices.push_back(offset + 0);
        indices.push_back(offset + 1);
        indices.push_back(offset + 2);
        indices.push_back(offset + 2);
        indices.push_back(offset + 3);
        indices.push_back(offset + 0);
      }

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

      // Position attribute
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

      // TexCoord attribute
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

      // Color attribute
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

      // Parallax factor attribute
      glEnableVertexAttribArray(3);
      glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, parallax_factor));

      glBindVertexArray(0);

      // Setup OpenGL state
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      // Initialize camera to default
      reset_camera();

      // Reserve space for batch vertices
      batch_state.vertices.reserve(MAX_VERTEX_COUNT);

      Log::info("Renderer initialized with batching (max " + std::to_string(MAX_QUAD_COUNT) + " quads per batch)");
      Log::info("Virtual resolution: " + std::to_string((int)virtual_width) + "x" + std::to_string((int)virtual_height));
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

    void set_viewport(const Viewport& viewport) {
      current_viewport = viewport;
      glViewport(current_viewport.x, current_viewport.y, current_viewport.width, current_viewport.height);
    }

    Viewport get_viewport() {
      return current_viewport;
    }

    Math::Vec2D screen_to_virtual(const Math::Vec2D& screen_pos) {
      // Account for viewport offset
      float adjusted_x = screen_pos.x - current_viewport.x;
      float adjusted_y = screen_pos.y - current_viewport.y;

      // Convert from screen pixels to virtual resolution coordinates
      float scale_x = virtual_width / current_viewport.width;
      float scale_y = virtual_height / current_viewport.height;

      return Math::Vec2D(
        adjusted_x * scale_x,
        adjusted_y * scale_y
      );
    }

    Math::Vec2D virtual_to_screen(const Math::Vec2D& virtual_pos) {
      // Convert from virtual resolution to screen pixels
      float scale_x = current_viewport.width / virtual_width;
      float scale_y = current_viewport.height / virtual_height;

      return Math::Vec2D(
        virtual_pos.x * scale_x + current_viewport.x,
        virtual_pos.y * scale_y + current_viewport.y
      );
    }

    Math::Vec2D virtual_to_world(const Math::Vec2D& virtual_pos) {
      // To invert the view transformation, we need to reverse the order
      // Original: translate to center -> rotate -> scale -> translate from center -> translate camera
      // Inverse: translate camera inverse -> translate to center -> inverse scale -> inverse rotate -> translate from center

      float center_x = virtual_width * 0.5f;
      float center_y = virtual_height * 0.5f;

      glm::vec4 pos = glm::vec4(virtual_pos.x, virtual_pos.y, 0.0f, 1.0f);
      glm::mat4 inverse_view = glm::mat4(1.0f);

      // Step 1: Reverse the camera position translation
      inverse_view = glm::translate(inverse_view, glm::vec3(current_camera.position.x, current_camera.position.y, 0.0f));

      // Step 2: Translate to center
      inverse_view = glm::translate(inverse_view, glm::vec3(center_x, center_y, 0.0f));

      // Step 3: Inverse scale (zoom)
      if (current_camera.zoom != 0.0f) {
        inverse_view = glm::scale(inverse_view, glm::vec3(1.0f / current_camera.zoom, 1.0f / current_camera.zoom, 1.0f));
      }

      // Step 4: Inverse rotation (negate the angle)
      if (current_camera.rotation != 0.0f) {
        inverse_view = glm::rotate(inverse_view, glm::radians(current_camera.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      }

      // Step 5: Translate back from center
      inverse_view = glm::translate(inverse_view, glm::vec3(-center_x, -center_y, 0.0f));

      glm::vec4 world_pos = inverse_view * pos;
      return Math::Vec2D(world_pos.x, world_pos.y);
    }

    Math::Vec2D world_to_virtual(const Math::Vec2D& world_pos) {
      glm::vec4 pos = glm::vec4(world_pos.x, world_pos.y, 0.0f, 1.0f);
      glm::mat4 view = get_view_matrix();
      glm::vec4 virtual_pos = view * pos;
      return Math::Vec2D(virtual_pos.x, virtual_pos.y);
    }

    Math::Vec2D screen_to_world(const Math::Vec2D& screen_pos) {
      // First convert screen to virtual, then virtual to world
      Math::Vec2D virtual_pos = screen_to_virtual(screen_pos);
      return virtual_to_world(virtual_pos);
    }

    Math::Vec2D world_to_screen(const Math::Vec2D& world_pos) {
      // First convert world to virtual, then virtual to screen
      Math::Vec2D virtual_pos = world_to_virtual(world_pos);
      return virtual_to_screen(virtual_pos);
    }

    void draw_rect(const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, const Math::Vec2D& parallax_factor)
    {
      draw_texture(Engine::Resources::get_rect_texture(), position, size, rotation, color, parallax_factor);
    }

    void draw_texture(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, const Math::Vec2D& parallax_factor) {
      // Default texture coordinates for full texture (bottom-left, bottom-right, top-right, top-left)
      std::array<Math::Vec2D, 4> default_tex_coords = {
        Math::Vec2D(0.0f, 1.0f), // Bottom-left
        Math::Vec2D(1.0f, 1.0f), // Bottom-right
        Math::Vec2D(1.0f, 0.0f), // Top-right
        Math::Vec2D(0.0f, 0.0f)  // Top-left
      };

      draw_quad(texture, position, size, rotation, color, default_tex_coords, glm::vec2(parallax_factor.x, parallax_factor.y));
    }

    void draw_subtexture(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords,
      const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, const Math::Vec2D& parallax_factor) {
      draw_quad(texture, position, size, rotation, color, tex_coords, glm::vec2(parallax_factor.x, parallax_factor.y));
    }

    void draw_text(Engine::Graphics::Font font, const std::string& text, const Math::Vec2D& position, f32 scale, const Math::Vec4D& color, const Math::Vec2D& parallax_factor) {
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
          continue;
        }

        const Glyph& glyph = it->second;

        // Calculate position and size for this glyph
        float xpos = x + glyph.bearing.x * scale;
        float ypos = y + (font->baseline - glyph.bearing.y) * scale;

        float w = glyph.size.x * scale;
        float h = glyph.size.y * scale;

        // Draw the glyph with parallax factor
        draw_glyph(font->texture_atlas, glyph.tex_coords,
          Math::Vec2D(xpos, ypos), Math::Vec2D(w, h), color, glm::vec2(parallax_factor.x, parallax_factor.y));

        // Advance cursor to next glyph position
        x += (glyph.advance >> 6) * scale;
      }
    }

    void clear_screen(const Math::Vec4D& color) {
      // Flush any pending batches before clearing
      end_batch();

      glClearColor(color.x, color.y, color.z, color.w);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // Begin new batch
      begin_batch();
    }

    void clear_screen(Engine::Graphics::Texture texture) {
      // Flush any pending batches before clearing
      end_batch();

      // Clear with black first
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // Begin new batch
      begin_batch();

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
