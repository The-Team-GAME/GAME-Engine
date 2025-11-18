//-----------------------------------------------------------------------------------
// file:    Renderer.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Core graphics rendering implementation for the game engine.
//          Handles OpenGL rendering pipeline, shader management, batching,
//          camera transformations, and virtual resolution scaling. Provides
//          drawing functions for textures, sprites, text, and distortion effects.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
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
#include "../GUI/ViewportPanel.h"
#include "../FileIO/Settings.h"

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
      f32 distortion_mesh_intensity = 0.0f;

      void reset() {
        vertices.clear();
        current_texture_id = 0;
        quad_count = 0;
        current_shader = nullptr;
        distortion_intensity = 0.0f;
        distortion_speed = 1.0f;
        distortion_mesh_intensity = 0.0f;
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
        active_shader->set_float("u_mesh_intensity", batch_state.distortion_mesh_intensity);
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
    void batch_quad(GLuint texture_id, const Vertex vertices[4], Shader* use_shader = nullptr, f32 intensity = 0.0f, f32 speed = 1.0f, f32 mesh_intensity = 0.0f) {
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
      batch_state.distortion_mesh_intensity = mesh_intensity;

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

    // UI-specific helper functions (no camera transformation)

    // Helper function to draw a quad in UI space (no view matrix applied)
    void draw_ui_quad(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size,
      f32 rotation, const Math::Vec4D& color, const std::array<Math::Vec2D, 4>& tex_coords) {
      if (!texture || texture->id == 0) {
        Log::warning("Attempted to draw invalid texture");
        return;
      }

      if (!shader || !shader->is_valid()) {
        Log::error("Shader is not valid");
        return;
      }

      // Flush current batch if texture changed
      if (batch_state.current_texture_id != 0 && batch_state.current_texture_id != texture->id) {
        flush_batch();
      }

      // Check if we need to flush due to batch size
      if (batch_state.quad_count >= MAX_QUAD_COUNT) {
        flush_batch();
      }

      // Set current texture
      batch_state.current_texture_id = texture->id;

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

      // Transform vertices by model matrix only (no view matrix)
      for (int i = 0; i < 4; i++) {
        glm::vec4 transformed = model * glm::vec4(base_positions[i], 0.0f, 1.0f);
        vertices[i].position = glm::vec2(transformed.x, transformed.y);
        vertices[i].texCoord = glm::vec2(tex_coords[i].x, tex_coords[i].y);
        vertices[i].color = normalized_color;
      }

      // Manually add vertices to batch (avoiding camera transformation)
      // Reserve space for new vertices
      size_t vertex_offset = batch_state.vertices.size();
      batch_state.vertices.resize(vertex_offset + 4);

      for (int i = 0; i < 4; i++) {
        batch_state.vertices[vertex_offset + i] = vertices[i];
      }

      batch_state.quad_count++;
    }

    // Helper function to draw a UI glyph (character) at a specific position without camera transformation
    void draw_ui_glyph(const TextureData& atlas_texture, const std::array<Math::Vec2D, 4>& tex_coords,
      const Math::Vec2D& position, const Math::Vec2D& size, const Math::Vec4D& color) {
      if (atlas_texture.id == 0) {
        return;
      }

      if (!shader || !shader->is_valid()) {
        return;
      }

      // Flush current batch if texture changed
      if (batch_state.current_texture_id != 0 && batch_state.current_texture_id != atlas_texture.id) {
        flush_batch();
      }

      // Check if we need to flush due to batch size
      if (batch_state.quad_count >= MAX_QUAD_COUNT) {
        flush_batch();
      }

      // Set current texture
      batch_state.current_texture_id = atlas_texture.id;

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

      // Manually add vertices to batch (avoiding camera transformation)
      size_t vertex_offset = batch_state.vertices.size();
      batch_state.vertices.resize(vertex_offset + 4);

      for (int i = 0; i < 4; i++) {
        batch_state.vertices[vertex_offset + i] = vertices[i];
      }

      batch_state.quad_count++;
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
      texture_data->path = "UNIT_RECT";

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

    std::pair<Math::Vec2D, bool> screen_to_viewport(const Math::Vec2D& screen_pos) {

      if (!Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().imgui_viewport_enabled) {
        // This means that imgui viewport mode is disabled, so we can just return the virtual coordinates
        return { screen_to_virtual(screen_pos), true };
      }

      // Get viewport bounds from ViewportPanel (x: min_x, y: min_y, z: max_x, w: max_y)
      Math::Vec4D bounds = ViewportPanel::get_viewport_bounds();

      // Calculate viewport dimensions
      float viewport_width = bounds.z - bounds.x;
      float viewport_height = bounds.w - bounds.y;

      // Convert screen position to viewport-relative position
      float relative_x = screen_pos.x - bounds.x;
      float relative_y = screen_pos.y - bounds.y;

      // Normalize to [0, 1] range
      float normalized_x = relative_x / viewport_width;
      float normalized_y = relative_y / viewport_height;

      // Check if mouse is within viewport bounds
      bool is_in_viewport = (normalized_x >= 0.0f && normalized_x <= 1.0f &&
        normalized_y >= 0.0f && normalized_y <= 1.0f);

      // Clamp to viewport bounds
      // normalized_x = std::max(0.0f, std::min(1.0f, normalized_x));
      // normalized_y = std::max(0.0f, std::min(1.0f, normalized_y));

      // Convert to virtual coordinates
      Math::Vec2D virtual_pos;
      virtual_pos.x = normalized_x * virtual_width;
      virtual_pos.y = normalized_y * virtual_height;

      // std::cout << "Viewport Mouse Position: (" << virtual_pos.x << ", " << virtual_pos.y << ")\n";

      return { virtual_pos, is_in_viewport };
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
      // Flush current batch before drawing lines
      flush_batch();

      // Enable line rendering
      glLineWidth(thickness);

      // Set up shader
      shader->use();
      shader->set_mat4("u_projection", get_projection_matrix());
      shader->set_mat4("u_view", get_view_matrix());
      shader->set_int("u_texture", 0);

      // Bind white texture for solid color
      glBindTexture(GL_TEXTURE_2D, unit_rect->id);

      // Create line vertices
      Vertex line_vertices[2];
      line_vertices[0].position = glm::vec2(start.x, start.y);
      line_vertices[0].texCoord = glm::vec2(0.5f, 0.5f);
      line_vertices[0].color = glm::vec4(color.x, color.y, color.z, color.w);

      line_vertices[1].position = glm::vec2(end.x, end.y);
      line_vertices[1].texCoord = glm::vec2(0.5f, 0.5f);
      line_vertices[1].color = glm::vec4(color.x, color.y, color.z, color.w);

      // Upload vertices
      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line_vertices), line_vertices);

      // Draw line
      glDrawArrays(GL_LINES, 0, 2);

      // Reset line width to default
      glLineWidth(1.0f);
    }

    void draw_polygon(const std::vector<Math::Vec2D>& vertices, const Math::Vec2D& position, f32 rotation, const Math::Vec4D& color) {
      if (vertices.size() < 3) {
        Log::warning("Attempted to draw polygon with less than 3 vertices");
        return;
      }

      // Convert rotation to radians
      f32 angle_rad = rotation * 3.14159265359f / 180.0f;
      f32 cos_angle = std::cos(angle_rad);
      f32 sin_angle = std::sin(angle_rad);

      // Transform and rotate vertices
      std::vector<Math::Vec2D> transformed_vertices;
      transformed_vertices.reserve(vertices.size());

      for (const auto& vertex : vertices) {
        // Apply rotation around origin
        f32 rotated_x = vertex.x * cos_angle - vertex.y * sin_angle;
        f32 rotated_y = vertex.x * sin_angle + vertex.y * cos_angle;

        // Translate to position
        transformed_vertices.push_back(Math::Vec2D(rotated_x + position.x, rotated_y + position.y));
      }

      // Get the unit rect texture for drawing filled triangles
      auto texture = Engine::Resources::get_rect_texture();
      if (!texture || texture->id == 0) {
        return;
      }

      // Calculate centroid of the polygon
      Math::Vec2D centroid(0.0f, 0.0f);
      for (const auto& vertex : transformed_vertices) {
        centroid = centroid + vertex;
      }
      centroid = centroid * (1.0f / transformed_vertices.size());

      // Triangulate from centroid to each edge
      // This works for both convex and concave polygons (like stars)
      for (size_t i = 0; i < transformed_vertices.size(); ++i) {
        size_t next_i = (i + 1) % transformed_vertices.size();

        Math::Vec2D v0 = centroid;
        Math::Vec2D v1 = transformed_vertices[i];
        Math::Vec2D v2 = transformed_vertices[next_i];

        // Create vertices for a filled triangle using the quad batching system
        Vertex tri_vertices[4];

        // Convert color from 0-1 range
        glm::vec4 normalized_color(color.x, color.y, color.z, color.w);

        // Use the three triangle vertices and repeat the last one to make a quad
        // This creates a degenerate quad that approximates the triangle
        tri_vertices[0].position = glm::vec2(v0.x, v0.y);
        tri_vertices[0].texCoord = glm::vec2(0.5f, 0.5f);  // Center texture
        tri_vertices[0].color = normalized_color;

        tri_vertices[1].position = glm::vec2(v1.x, v1.y);
        tri_vertices[1].texCoord = glm::vec2(0.5f, 0.5f);
        tri_vertices[1].color = normalized_color;

        tri_vertices[2].position = glm::vec2(v2.x, v2.y);
        tri_vertices[2].texCoord = glm::vec2(0.5f, 0.5f);
        tri_vertices[2].color = normalized_color;

        tri_vertices[3].position = glm::vec2(v2.x, v2.y);  // Repeat last vertex
        tri_vertices[3].texCoord = glm::vec2(0.5f, 0.5f);
        tri_vertices[3].color = normalized_color;

        // Add to batch
        batch_quad(texture->id, tri_vertices);
      }
    }

    void draw_triangle(const Math::Vec2D& position, f32 radius, f32 rotation, const Math::Vec4D& color) {
      // Create vertices for an equilateral triangle centered at origin
      std::vector<Math::Vec2D> vertices;
      vertices.reserve(3);

      // Triangle points at 0°, 120°, and 240° (pointing up by default)
      for (int i = 0; i < 3; ++i) {
        f32 angle = (i * 120.0f - 90.0f) * 3.14159265359f / 180.0f; // -90 to point up
        vertices.push_back(Math::Vec2D(
          std::cos(angle) * radius,
          std::sin(angle) * radius
        ));
      }

      draw_polygon(vertices, position, rotation, color);
    }

    void draw_circle(const Math::Vec2D& position, f32 radius, const Math::Vec4D& color, int segments) {
      if (segments < 3) {
        segments = 3; // Minimum for a visible shape
      }

      if (radius <= 0.0f) {
        return; // Invalid radius
      }

      // Create vertices for a circle
      std::vector<Math::Vec2D> vertices;
      vertices.reserve(segments);

      f32 angle_step = (2.0f * 3.14159265359f) / segments;

      for (int i = 0; i < segments; ++i) {
        f32 angle = i * angle_step;
        vertices.push_back(Math::Vec2D(
          std::cos(angle) * radius,
          std::sin(angle) * radius
        ));
      }

      draw_polygon(vertices, position, 0.0f, color);
    }

    void draw_polygon_outline(const std::vector<Math::Vec2D>& vertices, const Math::Vec2D& position, f32 rotation, f32 thickness, const Math::Vec4D& color) {
      if (vertices.size() < 3) {
        Log::warning("Attempted to draw polygon outline with less than 3 vertices");
        return;
      }

      // Convert rotation to radians
      f32 angle_rad = rotation * 3.14159265359f / 180.0f;
      f32 cos_angle = std::cos(angle_rad);
      f32 sin_angle = std::sin(angle_rad);

      // Transform and rotate vertices
      std::vector<Math::Vec2D> transformed_vertices;
      transformed_vertices.reserve(vertices.size());

      for (const auto& vertex : vertices) {
        // Apply rotation around origin
        f32 rotated_x = vertex.x * cos_angle - vertex.y * sin_angle;
        f32 rotated_y = vertex.x * sin_angle + vertex.y * cos_angle;

        // Translate to position
        transformed_vertices.push_back(Math::Vec2D(rotated_x + position.x, rotated_y + position.y));
      }

      // Draw lines between consecutive vertices
      for (size_t i = 0; i < transformed_vertices.size(); ++i) {
        size_t next_i = (i + 1) % transformed_vertices.size();
        draw_line(transformed_vertices[i], transformed_vertices[next_i], thickness, color);
      }
    }

    void draw_triangle_outline(const Math::Vec2D& position, f32 radius, f32 rotation, f32 thickness, const Math::Vec4D& color) {
      // Create vertices for an equilateral triangle centered at origin
      std::vector<Math::Vec2D> vertices;
      vertices.reserve(3);

      // Triangle points at 0°, 120°, and 240° (pointing up by default)
      for (int i = 0; i < 3; ++i) {
        f32 angle = (i * 120.0f - 90.0f) * 3.14159265359f / 180.0f; // -90 to point up
        vertices.push_back(Math::Vec2D(
          std::cos(angle) * radius,
          std::sin(angle) * radius
        ));
      }

      draw_polygon_outline(vertices, position, rotation, thickness, color);
    }

    void draw_circle_outline(const Math::Vec2D& position, f32 radius, f32 thickness, const Math::Vec4D& color, int segments) {
      if (segments < 3) {
        segments = 3; // Minimum for a visible shape
      }

      if (radius <= 0.0f) {
        return; // Invalid radius
      }

      // Create vertices for a circle
      std::vector<Math::Vec2D> vertices;
      vertices.reserve(segments);

      f32 angle_step = (2.0f * 3.14159265359f) / segments;

      for (int i = 0; i < segments; ++i) {
        f32 angle = i * angle_step;
        vertices.push_back(Math::Vec2D(
          std::cos(angle) * radius,
          std::sin(angle) * radius
        ));
      }

      draw_polygon_outline(vertices, position, 0.0f, thickness, color);
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

    void draw_subtexture_tiled(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords,
      const Math::Vec2D& position, const Math::Vec2D& size, const Math::Vec2D& tile_size, f32 rotation, const Math::Vec4D& color) {

      // For tiled rendering, we need to draw multiple instances of the same tile
      // Calculate how many tiles we need in each direction
      int num_tiles_x = static_cast<int>(std::ceil(size.x / tile_size.x));
      int num_tiles_y = static_cast<int>(std::ceil(size.y / tile_size.y));

      // Calculate top-left corner of the tiled area
      f32 start_x = position.x - size.x * 0.5f;
      f32 start_y = position.y - size.y * 0.5f;

      // Draw each tile instance
      for (int y = 0; y < num_tiles_y; ++y) {
        for (int x = 0; x < num_tiles_x; ++x) {
          // Calculate position for this tile
          f32 tile_x = start_x + x * tile_size.x + tile_size.x * 0.5f;
          f32 tile_y = start_y + y * tile_size.y + tile_size.y * 0.5f;

          // Draw the tile at this position
          draw_quad(texture, Math::Vec2D(tile_x, tile_y), tile_size, rotation, color, tex_coords);
        }
      }
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

    Math::Vec2D measure_text(Engine::Graphics::Font font, const std::string& text, f32 scale) {
      if (!font || font->texture_atlas.id == 0) {
        Log::warning("Attempted to measure text with invalid font");
        return Math::Vec2D(0.0f, 0.0f);
      }

      if (text.empty()) {
        return Math::Vec2D(0.0f, 0.0f);
      }

      float width = 0.0f;
      float height = font->baseline * scale; // Use baseline as the standard height

      // Iterate through all characters in the text
      for (char c : text) {
        // Get the glyph for this character
        auto it = font->glyphs.find(static_cast<unsigned int>(c));

        if (it == font->glyphs.end()) {
          continue;
        }

        const Glyph& glyph = it->second;

        // Advance cursor to next glyph position
        width += (glyph.advance >> 6) * scale;

        // Track the maximum height needed
        float glyph_height = glyph.size.y * scale;
        if (glyph_height > height) {
          height = glyph_height;
        }
      }

      return Math::Vec2D(width, height);
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
      f32 intensity, f32 speed, f32 mesh_intensity) {
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
      batch_quad(texture->id, vertices, distortion_shader.get(), intensity, speed, mesh_intensity);
    }

    void draw_texture_distortion(Engine::Graphics::Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, f32 intensity, f32 speed, f32 mesh_intensity) {
      // Default texture coordinates (full texture)
      std::array<Math::Vec2D, 4> tex_coords = {
        Math::Vec2D(0.0f, 1.0f), // Bottom-left
        Math::Vec2D(1.0f, 1.0f), // Bottom-right
        Math::Vec2D(1.0f, 0.0f), // Top-right
        Math::Vec2D(0.0f, 0.0f)  // Top-left
      };

      draw_quad_distortion(texture, position, size, rotation, color, tex_coords, intensity, speed, mesh_intensity);
    }

    void draw_subtexture_distortion(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, f32 intensity, f32 speed, f32 mesh_intensity) {
      draw_quad_distortion(texture, position, size, rotation, color, tex_coords, intensity, speed, mesh_intensity);
    }

    // UI Drawing functions (no camera transformation)

    void draw_rect_ui(const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color)
    {
      draw_texture_ui(Engine::Resources::get_rect_texture(), position, size, rotation, color);
    }

    void draw_rect_outline_ui(const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, f32 thickness, const Math::Vec4D& color) {
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
        draw_line_ui(corners[0], corners[1], thickness, color); // Top
        draw_line_ui(corners[1], corners[2], thickness, color); // Right
        draw_line_ui(corners[2], corners[3], thickness, color); // Bottom
        draw_line_ui(corners[3], corners[0], thickness, color); // Left
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
        draw_line_ui(top_left, top_right, thickness, color);      // Top
        draw_line_ui(top_right, bottom_right, thickness, color);  // Right
        draw_line_ui(bottom_right, bottom_left, thickness, color); // Bottom
        draw_line_ui(bottom_left, top_left, thickness, color);    // Left
      }
    }

    void draw_line_ui(const Math::Vec2D& start, const Math::Vec2D& end, f32 thickness, const Math::Vec4D& color) {
      // Flush current batch before drawing lines
      flush_batch();

      // Enable line rendering
      glLineWidth(thickness);

      // Set up shader (no view matrix for UI)
      shader->use();
      shader->set_mat4("u_projection", get_projection_matrix());
      shader->set_mat4("u_view", glm::mat4(1.0f)); // Identity matrix - no camera transformation
      shader->set_int("u_texture", 0);

      // Bind white texture for solid color
      glBindTexture(GL_TEXTURE_2D, unit_rect->id);

      // Create line vertices
      Vertex line_vertices[2];
      line_vertices[0].position = glm::vec2(start.x, start.y);
      line_vertices[0].texCoord = glm::vec2(0.5f, 0.5f);
      line_vertices[0].color = glm::vec4(color.x, color.y, color.z, color.w);

      line_vertices[1].position = glm::vec2(end.x, end.y);
      line_vertices[1].texCoord = glm::vec2(0.5f, 0.5f);
      line_vertices[1].color = glm::vec4(color.x, color.y, color.z, color.w);

      // Upload vertices
      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line_vertices), line_vertices);

      // Draw line
      glDrawArrays(GL_LINES, 0, 2);

      // Reset line width to default
      glLineWidth(1.0f);
    }

    void draw_polygon_ui(const std::vector<Math::Vec2D>& vertices, const Math::Vec2D& position, f32 rotation, const Math::Vec4D& color) {
      if (vertices.size() < 3) {
        Log::warning("Attempted to draw UI polygon with less than 3 vertices");
        return;
      }

      // Convert rotation to radians
      f32 angle_rad = rotation * 3.14159265359f / 180.0f;
      f32 cos_angle = std::cos(angle_rad);
      f32 sin_angle = std::sin(angle_rad);

      // Transform and rotate vertices
      std::vector<Math::Vec2D> transformed_vertices;
      transformed_vertices.reserve(vertices.size());

      for (const auto& vertex : vertices) {
        // Apply rotation around origin
        f32 rotated_x = vertex.x * cos_angle - vertex.y * sin_angle;
        f32 rotated_y = vertex.x * sin_angle + vertex.y * cos_angle;

        // Translate to position
        transformed_vertices.push_back(Math::Vec2D(rotated_x + position.x, rotated_y + position.y));
      }

      // Get the unit rect texture for drawing filled triangles
      auto texture = Engine::Resources::get_rect_texture();
      if (!texture || texture->id == 0) {
        return;
      }

      // Calculate centroid of the polygon
      Math::Vec2D centroid(0.0f, 0.0f);
      for (const auto& vertex : transformed_vertices) {
        centroid = centroid + vertex;
      }
      centroid = centroid * (1.0f / transformed_vertices.size());

      // Triangulate from centroid to each edge
      for (size_t i = 0; i < transformed_vertices.size(); ++i) {
        size_t next_i = (i + 1) % transformed_vertices.size();

        Math::Vec2D v0 = centroid;
        Math::Vec2D v1 = transformed_vertices[i];
        Math::Vec2D v2 = transformed_vertices[next_i];

        // Create vertices for a filled triangle using the quad batching system
        Vertex tri_vertices[4];

        // Convert color from 0-1 range
        glm::vec4 normalized_color(color.x, color.y, color.z, color.w);

        // Use the three triangle vertices and repeat the last one to make a quad
        tri_vertices[0].position = glm::vec2(v0.x, v0.y);
        tri_vertices[0].texCoord = glm::vec2(0.5f, 0.5f);
        tri_vertices[0].color = normalized_color;

        tri_vertices[1].position = glm::vec2(v1.x, v1.y);
        tri_vertices[1].texCoord = glm::vec2(0.5f, 0.5f);
        tri_vertices[1].color = normalized_color;

        tri_vertices[2].position = glm::vec2(v2.x, v2.y);
        tri_vertices[2].texCoord = glm::vec2(0.5f, 0.5f);
        tri_vertices[2].color = normalized_color;

        tri_vertices[3].position = glm::vec2(v2.x, v2.y);
        tri_vertices[3].texCoord = glm::vec2(0.5f, 0.5f);
        tri_vertices[3].color = normalized_color;

        // Flush current batch if texture changed
        if (batch_state.current_texture_id != 0 && batch_state.current_texture_id != texture->id) {
          flush_batch();
        }

        // Check if we need to flush due to batch size
        if (batch_state.quad_count >= MAX_QUAD_COUNT) {
          flush_batch();
        }

        // Set current texture
        batch_state.current_texture_id = texture->id;

        // Manually add vertices to batch (avoiding camera transformation)
        size_t vertex_offset = batch_state.vertices.size();
        batch_state.vertices.resize(vertex_offset + 4);

        for (int j = 0; j < 4; j++) {
          batch_state.vertices[vertex_offset + j] = tri_vertices[j];
        }

        batch_state.quad_count++;
      }
    }

    void draw_triangle_ui(const Math::Vec2D& position, f32 radius, f32 rotation, const Math::Vec4D& color) {
      // Create vertices for an equilateral triangle centered at origin
      std::vector<Math::Vec2D> vertices;
      vertices.reserve(3);

      // Triangle points at 0°, 120°, and 240° (pointing up by default)
      for (int i = 0; i < 3; ++i) {
        f32 angle = (i * 120.0f - 90.0f) * 3.14159265359f / 180.0f;
        vertices.push_back(Math::Vec2D(
          std::cos(angle) * radius,
          std::sin(angle) * radius
        ));
      }

      draw_polygon_ui(vertices, position, rotation, color);
    }

    void draw_circle_ui(const Math::Vec2D& position, f32 radius, const Math::Vec4D& color, int segments) {
      if (segments < 3) {
        segments = 3;
      }

      if (radius <= 0.0f) {
        return;
      }

      // Create vertices for a circle
      std::vector<Math::Vec2D> vertices;
      vertices.reserve(segments);

      f32 angle_step = (2.0f * 3.14159265359f) / segments;

      for (int i = 0; i < segments; ++i) {
        f32 angle = i * angle_step;
        vertices.push_back(Math::Vec2D(
          std::cos(angle) * radius,
          std::sin(angle) * radius
        ));
      }

      draw_polygon_ui(vertices, position, 0.0f, color);
    }

    void draw_polygon_outline_ui(const std::vector<Math::Vec2D>& vertices, const Math::Vec2D& position, f32 rotation, f32 thickness, const Math::Vec4D& color) {
      if (vertices.size() < 3) {
        Log::warning("Attempted to draw UI polygon outline with less than 3 vertices");
        return;
      }

      // Convert rotation to radians
      f32 angle_rad = rotation * 3.14159265359f / 180.0f;
      f32 cos_angle = std::cos(angle_rad);
      f32 sin_angle = std::sin(angle_rad);

      // Transform and rotate vertices
      std::vector<Math::Vec2D> transformed_vertices;
      transformed_vertices.reserve(vertices.size());

      for (const auto& vertex : vertices) {
        // Apply rotation around origin
        f32 rotated_x = vertex.x * cos_angle - vertex.y * sin_angle;
        f32 rotated_y = vertex.x * sin_angle + vertex.y * cos_angle;

        // Translate to position
        transformed_vertices.push_back(Math::Vec2D(rotated_x + position.x, rotated_y + position.y));
      }

      // Draw lines between consecutive vertices
      for (size_t i = 0; i < transformed_vertices.size(); ++i) {
        size_t next_i = (i + 1) % transformed_vertices.size();
        draw_line_ui(transformed_vertices[i], transformed_vertices[next_i], thickness, color);
      }
    }

    void draw_triangle_outline_ui(const Math::Vec2D& position, f32 radius, f32 rotation, f32 thickness, const Math::Vec4D& color) {
      // Create vertices for an equilateral triangle centered at origin
      std::vector<Math::Vec2D> vertices;
      vertices.reserve(3);

      // Triangle points at 0°, 120°, and 240° (pointing up by default)
      for (int i = 0; i < 3; ++i) {
        f32 angle = (i * 120.0f - 90.0f) * 3.14159265359f / 180.0f;
        vertices.push_back(Math::Vec2D(
          std::cos(angle) * radius,
          std::sin(angle) * radius
        ));
      }

      draw_polygon_outline_ui(vertices, position, rotation, thickness, color);
    }

    void draw_circle_outline_ui(const Math::Vec2D& position, f32 radius, f32 thickness, const Math::Vec4D& color, int segments) {
      if (segments < 3) {
        segments = 3;
      }

      if (radius <= 0.0f) {
        return;
      }

      // Create vertices for a circle
      std::vector<Math::Vec2D> vertices;
      vertices.reserve(segments);

      f32 angle_step = (2.0f * 3.14159265359f) / segments;

      for (int i = 0; i < segments; ++i) {
        f32 angle = i * angle_step;
        vertices.push_back(Math::Vec2D(
          std::cos(angle) * radius,
          std::sin(angle) * radius
        ));
      }

      draw_polygon_outline_ui(vertices, position, 0.0f, thickness, color);
    }

    void draw_texture_ui(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color) {
      // Default texture coordinates for full texture
      std::array<Math::Vec2D, 4> default_tex_coords = {
        Math::Vec2D(0.0f, 1.0f), // Bottom-left
        Math::Vec2D(1.0f, 1.0f), // Bottom-right
        Math::Vec2D(1.0f, 0.0f), // Top-right
        Math::Vec2D(0.0f, 0.0f)  // Top-left
      };

      draw_ui_quad(texture, position, size, rotation, color, default_tex_coords);
    }

    void draw_subtexture_ui(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords,
      const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color) {
      draw_ui_quad(texture, position, size, rotation, color, tex_coords);
    }

    void draw_tilemap(const std::unordered_map<int64_t, Tile>& tileMap,
      const std::vector<AtlasEntry>& loaded_tilemaps,
      const Math::Vec2D& tileSize,
      const Math::Vec2D& gridOffset,
      const Math::Vec4D& color) {

      if (tileMap.empty() || loaded_tilemaps.empty()) {
        return;
      }

      // Get bounds of the tilemap
      int min_x = INT_MAX, min_y = INT_MAX;
      int max_x = INT_MIN, max_y = INT_MIN;

      for (const auto& [key, tile] : tileMap) {
        if (!tile.is_valid()) continue;

        int x, y;
        // Decode key: y is upper 32 bits, x is lower 32 bits
        y = static_cast<int>(key >> 32);
        x = static_cast<int>(key & 0xFFFFFFFF);

        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
      }

      if (min_x == INT_MAX) {
        return; // No valid tiles
      }

      int width = max_x - min_x + 1;
      int height = max_y - min_y + 1;

      // Helper struct for rectangle merging
      struct Rectangle {
        int x, y, width, height;
        Tile tile;
      };

      // Track processed tiles
      std::vector<std::vector<bool>> processed(height, std::vector<bool>(width, false));
      std::vector<Rectangle> rectangles;

      // Helper lambda to get tile from hashmap
      auto get_tile = [&](int x, int y) -> Tile {
        int64_t key = (static_cast<int64_t>(y) << 32) | (static_cast<uint32_t>(x));
        auto it = tileMap.find(key);
        return (it != tileMap.end()) ? it->second : Tile();
        };

      // Find all rectangles using greedy algorithm
      // Tiles can only merge if they have the same atlas_index AND tile_index
      for (int rel_y = 0; rel_y < height; ++rel_y) {
        for (int rel_x = 0; rel_x < width; ++rel_x) {
          if (processed[rel_y][rel_x]) continue;

          int abs_x = min_x + rel_x;
          int abs_y = min_y + rel_y;
          Tile tile = get_tile(abs_x, abs_y);

          if (!tile.is_valid() || tile.atlas_index >= static_cast<int>(loaded_tilemaps.size())) {
            processed[rel_y][rel_x] = true;
            continue;
          }

          const auto& atlas_entry = loaded_tilemaps[tile.atlas_index];
          if (tile.tile_index >= static_cast<int>(atlas_entry.subTextures.size())) {
            processed[rel_y][rel_x] = true;
            continue;
          }

          // Find maximum width at this row (same tile only)
          int max_width = 0;
          for (int w = 0; rel_x + w < width; ++w) {
            Tile check_tile = get_tile(abs_x + w, abs_y);
            if (processed[rel_y][rel_x + w] ||
              check_tile.atlas_index != tile.atlas_index ||
              check_tile.tile_index != tile.tile_index) {
              break;
            }
            max_width++;
          }

          if (max_width == 0) {
            processed[rel_y][rel_x] = true;
            continue;
          }

          // Expand downward while maintaining the width
          int rect_height = 1;
          for (int h = 1; rel_y + h < height; ++h) {
            bool can_expand = true;
            for (int w = 0; w < max_width; ++w) {
              Tile check_tile = get_tile(abs_x + w, abs_y + h);
              if (processed[rel_y + h][rel_x + w] ||
                check_tile.atlas_index != tile.atlas_index ||
                check_tile.tile_index != tile.tile_index) {
                can_expand = false;
                break;
              }
            }
            if (!can_expand) break;
            rect_height++;
          }

          // Mark all tiles in this rectangle as processed
          for (int h = 0; h < rect_height; ++h) {
            for (int w = 0; w < max_width; ++w) {
              processed[rel_y + h][rel_x + w] = true;
            }
          }

          // Store rectangle with absolute coordinates and tile info
          rectangles.push_back({ abs_x, abs_y, max_width, rect_height, tile });
        }
      }

      // Draw each rectangle
      for (const Rectangle& rect : rectangles) {
        if (!rect.tile.is_valid() || rect.tile.atlas_index >= static_cast<int>(loaded_tilemaps.size())) {
          continue;
        }

        const auto& atlas_entry = loaded_tilemaps[rect.tile.atlas_index];
        if (rect.tile.tile_index >= static_cast<int>(atlas_entry.subTextures.size())) {
          continue;
        }

        const auto& subTexture = atlas_entry.subTextures[rect.tile.tile_index];

        // Calculate the world position for the center of the rectangle
        f32 center_x = gridOffset.x + rect.x * tileSize.x + (rect.width * tileSize.x) * 0.5f;
        f32 center_y = gridOffset.y + rect.y * tileSize.y + (rect.height * tileSize.y) * 0.5f;
        Math::Vec2D center_world = { center_x, center_y };
        Math::Vec2D scale = { rect.width * tileSize.x, rect.height * tileSize.y };

        // Draw the merged rectangle as a single tiled subtexture
        draw_subtexture_tiled(atlas_entry.texture_handle, subTexture.tex_coords, center_world, scale, tileSize, 0.0f, color);
      }
    }

    void draw_text_ui(Engine::Graphics::Font font, const std::string& text, const Math::Vec2D& position, f32 scale, const Math::Vec4D& color) {
      if (!font || font->texture_atlas.id == 0) {
        Log::warning("Attempted to draw UI text with invalid font");
        return;
      }

      if (text.empty()) {
        return;
      }

      f32 x = position.x;
      f32 y = position.y;

      // Iterate through all characters
      for (char c : text) {
        auto it = font->glyphs.find(c);
        if (it == font->glyphs.end()) {
          continue; // Skip characters not in font
        }

        const Graphics::Glyph& glyph = it->second;

        f32 xpos = x + glyph.bearing.x * scale;
        f32 ypos = y + (font->baseline - glyph.bearing.y) * scale;

        f32 w = glyph.size.x * scale;
        f32 h = glyph.size.y * scale;

        // Draw the glyph using UI rendering (no camera transformation)
        draw_ui_glyph(font->texture_atlas, glyph.tex_coords, Math::Vec2D(xpos, ypos), Math::Vec2D(w, h), color);

        // Advance cursor to next glyph
        x += (glyph.advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
      }
    }

  } // namespace Graphics
} // namespace Engine
