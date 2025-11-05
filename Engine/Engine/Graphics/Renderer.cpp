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
#include "../Time.h"

namespace Engine {
  namespace Graphics {

    // Shader programs
    std::unique_ptr<Shader> shader;
    std::unique_ptr<Shader> distortion_shader;

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
    };

    // Batch state
    struct BatchState {
      std::vector<Vertex> vertices;
      GLuint current_texture_id = 0;
      size_t quad_count = 0;
      Shader* current_shader = nullptr;
      f32 distortion_intensity = 0.0f;
      f32 distortion_speed = 1.0f;

      void reset() {
        vertices.clear();
        current_texture_id = 0;
        quad_count = 0;
        current_shader = nullptr;
        distortion_intensity = 0.0f;
        distortion_speed = 1.0f;
      }
    };

    BatchState batch_state;

    Viewport current_viewport;

    // Unit rectangle texture (1x1 white pixel)
    Texture unit_rect;

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

      // Use the appropriate shader
      Shader* active_shader = batch_state.current_shader ? batch_state.current_shader : shader.get();
      active_shader->use();

      // Bind texture
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, batch_state.current_texture_id);
      active_shader->set_int("u_texture", 0);

      // Set view and projection matrices
      active_shader->set_mat4("u_view", get_view_matrix());
      active_shader->set_mat4("u_projection", get_projection_matrix());

      // If using distortion shader, set distortion uniforms
      if (active_shader == distortion_shader.get()) {
        active_shader->set_float("u_time", Engine::Time::get_time());
        active_shader->set_float("u_intensity", batch_state.distortion_intensity);
        active_shader->set_float("u_speed", batch_state.distortion_speed);
      }

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
    void batch_quad(GLuint texture_id, const Vertex vertices[4], Shader* use_shader = nullptr, f32 intensity = 0.0f, f32 speed = 1.0f) {
      // Check if we need to flush due to texture or shader change
      if ((batch_state.current_texture_id != 0 && batch_state.current_texture_id != texture_id) ||
        (batch_state.current_shader != use_shader)) {
        flush_batch();
      }

      // Check if we need to flush due to batch being full
      if (batch_state.quad_count >= MAX_QUAD_COUNT) {
        flush_batch();
      }

      // Set current texture and shader
      batch_state.current_texture_id = texture_id;
      batch_state.current_shader = use_shader;
      batch_state.distortion_intensity = intensity;
      batch_state.distortion_speed = speed;

      // Add vertices to batch
      for (int i = 0; i < 4; i++) {
        batch_state.vertices.push_back(vertices[i]);
      }

      batch_state.quad_count++;
    }

    // Helper function to draw a quad with custom texture coordinates
    void draw_quad(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size,
      f32 rotation, const Math::Vec4D& color, const std::array<Math::Vec2D, 4>& tex_coords) {
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
      }

      // Add to batch instead of drawing immediately
      batch_quad(texture->id, vertices);
    }

    // Helper function to draw a glyph (character) at a specific position without rotation
    void draw_glyph(const TextureData& atlas_texture, const std::array<Math::Vec2D, 4>& tex_coords,
      const Math::Vec2D& position, const Math::Vec2D& size, const Math::Vec4D& color) {
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

      // Bottom-right
      vertices[1].position = glm::vec2(position.x + size.x, position.y + size.y);
      vertices[1].texCoord = glm::vec2(tex_coords[1].x, tex_coords[1].y);
      vertices[1].color = normalized_color;

      // Top-right
      vertices[2].position = glm::vec2(position.x + size.x, position.y);
      vertices[2].texCoord = glm::vec2(tex_coords[2].x, tex_coords[2].y);
      vertices[2].color = normalized_color;

      // Top-left
      vertices[3].position = glm::vec2(position.x, position.y);
      vertices[3].texCoord = glm::vec2(tex_coords[3].x, tex_coords[3].y);
      vertices[3].color = normalized_color;

      // Add to batch instead of drawing immediately
      batch_quad(atlas_texture.id, vertices);
    }

    bool init() {
      // Load default shader from files
      shader = Shader::load_from_file("Assets/shaders/default.vert", "Assets/shaders/default.frag");

      if (!shader->is_valid()) {
        Log::error("Failed to create default shader");
        return false;
      }

      // Load distortion shader from files
      distortion_shader = Shader::load_from_file("Assets/shaders/distortion.vert", "Assets/shaders/distortion.frag");

      if (!distortion_shader->is_valid()) {
        Log::error("Failed to create distortion shader");
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

      glBindVertexArray(0);

      // Setup OpenGL state
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      // Initialize camera to default
      reset_camera();

      // Reserve space for batch vertices
      batch_state.vertices.reserve(MAX_VERTEX_COUNT);

      // Create a 1x1 white pixel texture for unit_rect
      unsigned char white_pixel[4] = { 255, 255, 255, 255 };

      auto texture_data = std::make_shared<TextureData>();
      texture_data->width = 1;
      texture_data->height = 1;

      glGenTextures(1, &texture_data->id);
      glBindTexture(GL_TEXTURE_2D, texture_data->id);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);

      unit_rect = texture_data;

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

      // Clean up unit_rect texture
      unit_rect.reset();

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

    void draw_rect(const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color)
    {
      draw_texture(Engine::Resources::get_rect_texture(), position, size, rotation, color);
    }

    void draw_rect_outline(const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, f32 thickness, const Math::Vec4D& color) {
      // If rotated, we need to draw lines around a rotated rectangle
      if (rotation != 0.0f) {
        // Calculate half sizes
        f32 half_width = size.x * 0.5f;
        f32 half_height = size.y * 0.5f;

        // Convert rotation to radians
        f32 angle_rad = rotation * 3.14159265359f / 180.0f;
        f32 cos_angle = std::cos(angle_rad);
        f32 sin_angle = std::sin(angle_rad);

        // Calculate the four corners relative to center
        Math::Vec2D corners[4];
        corners[0] = Math::Vec2D(-half_width, -half_height); // Top-left
        corners[1] = Math::Vec2D(half_width, -half_height);  // Top-right
        corners[2] = Math::Vec2D(half_width, half_height);   // Bottom-right
        corners[3] = Math::Vec2D(-half_width, half_height);  // Bottom-left

        // Rotate and translate corners
        for (int i = 0; i < 4; i++) {
          f32 rotated_x = corners[i].x * cos_angle - corners[i].y * sin_angle;
          f32 rotated_y = corners[i].x * sin_angle + corners[i].y * cos_angle;
          corners[i] = Math::Vec2D(rotated_x + position.x, rotated_y + position.y);
        }

        // Draw the four sides
        draw_line(corners[0], corners[1], thickness, color); // Top
        draw_line(corners[1], corners[2], thickness, color); // Right
        draw_line(corners[2], corners[3], thickness, color); // Bottom
        draw_line(corners[3], corners[0], thickness, color); // Left
      }
      else {
        // For non-rotated rectangles, we can optimize by drawing axis-aligned lines
        f32 half_width = size.x * 0.5f;
        f32 half_height = size.y * 0.5f;

        Math::Vec2D top_left = Math::Vec2D(position.x - half_width, position.y - half_height);
        Math::Vec2D top_right = Math::Vec2D(position.x + half_width, position.y - half_height);
        Math::Vec2D bottom_right = Math::Vec2D(position.x + half_width, position.y + half_height);
        Math::Vec2D bottom_left = Math::Vec2D(position.x - half_width, position.y + half_height);

        // Draw the four sides
        draw_line(top_left, top_right, thickness, color);      // Top
        draw_line(top_right, bottom_right, thickness, color);  // Right
        draw_line(bottom_right, bottom_left, thickness, color); // Bottom
        draw_line(bottom_left, top_left, thickness, color);    // Left
      }
    }

    void draw_line(const Math::Vec2D& start, const Math::Vec2D& end, f32 thickness, const Math::Vec4D& color) {
      // Calculate line vector and length
      Math::Vec2D line_vec = end - start;
      f32 length = line_vec.get_magnitude();

      if (length < 0.001f) {
        return; // Line is too short to draw
      }

      // Calculate angle of the line
      f32 angle = std::atan2(line_vec.y, line_vec.x);

      // Calculate center position of the line
      Math::Vec2D center = start + (line_vec * 0.5f);

      // Draw a rotated rectangle to represent the line
      draw_texture(Engine::Resources::get_rect_texture(), center, Math::Vec2D(length, thickness), angle * 180.0f / 3.14159265359f, color);
    }

    void draw_texture(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color) {
      // Default texture coordinates for full texture (bottom-left, bottom-right, top-right, top-left)
      std::array<Math::Vec2D, 4> default_tex_coords = {
        Math::Vec2D(0.0f, 1.0f), // Bottom-left
        Math::Vec2D(1.0f, 1.0f), // Bottom-right
        Math::Vec2D(1.0f, 0.0f), // Top-right
        Math::Vec2D(0.0f, 0.0f)  // Top-left
      };

      draw_quad(texture, position, size, rotation, color, default_tex_coords);
    }

    void draw_subtexture(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords,
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

    Texture get_rect_texture() {
      return unit_rect;
    }

    // Helper function to draw a quad with distortion effect
    void draw_quad_distortion(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size,
      f32 rotation, const Math::Vec4D& color, const std::array<Math::Vec2D, 4>& tex_coords,
      f32 intensity, f32 speed) {
      if (!texture || texture->id == 0) {
        Log::warning("Attempted to draw invalid texture");
        return;
      }

      if (!distortion_shader || !distortion_shader->is_valid()) {
        Log::error("Distortion shader is not valid");
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
      }

      // Add to batch with distortion shader
      batch_quad(texture->id, vertices, distortion_shader.get(), intensity, speed);
    }

    void draw_texture_distortion(Engine::Graphics::Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, f32 intensity, f32 speed) {
      // Default texture coordinates (full texture)
      std::array<Math::Vec2D, 4> tex_coords = {
        Math::Vec2D(0.0f, 1.0f), // Bottom-left
        Math::Vec2D(1.0f, 1.0f), // Bottom-right
        Math::Vec2D(1.0f, 0.0f), // Top-right
        Math::Vec2D(0.0f, 0.0f)  // Top-left
      };

      draw_quad_distortion(texture, position, size, rotation, color, tex_coords, intensity, speed);
    }

    void draw_subtexture_distortion(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, f32 intensity, f32 speed) {
      draw_quad_distortion(texture, position, size, rotation, color, tex_coords, intensity, speed);
    }

  } // namespace Graphics
} // namespace Engine
