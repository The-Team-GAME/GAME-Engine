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

    // Shader programs (instanced rendering)
    std::unique_ptr<Shader> shader;
    std::unique_ptr<Shader> distortion_shader;

    // Instanced rendering GPU resources
    GLuint vao = 0;
    GLuint quad_vbo = 0;
    GLuint ebo = 0;
    GLuint instance_vbo = 0;

    // Polygon rendering (direct GL_TRIANGLES — cannot use instanced quads for arbitrary triangles)
    std::unique_ptr<Shader> polygon_shader;
    GLuint polygon_vao = 0;
    GLuint polygon_vbo = 0;

    // Virtual resolution
    float virtual_width = 1600.0f;
    float virtual_height = 900.0f;

    // Camera state
    Camera current_camera;

    // Batch rendering constants
    constexpr size_t MAX_INSTANCE_COUNT = 10000;

    // Per-instance data for instanced rendering (96 bytes per instance)
    struct InstanceData {
      glm::mat4 model;    // Model/transform matrix (64 bytes)
      glm::vec4 color;    // Instance color + alpha (16 bytes)
      glm::vec2 uv_min;   // UV min corner for subtexture (8 bytes)
      glm::vec2 uv_max;   // UV max corner for subtexture (8 bytes)
    };

    // Batch state for instanced rendering
    struct BatchState {
      std::vector<InstanceData> instances;
      GLuint current_texture_id = 0;
      Shader* current_shader = nullptr;
      f32 distortion_intensity = 0.0f;
      f32 distortion_speed = 1.0f;
      f32 distortion_mesh_intensity = 0.0f;
      bool is_ui_batch = false;

      void reset() {
        instances.clear();
        current_texture_id = 0;
        current_shader = nullptr;
        distortion_intensity = 0.0f;
        distortion_speed = 1.0f;
        distortion_mesh_intensity = 0.0f;
        is_ui_batch = false;
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

    // Flush the current batch to GPU using instanced rendering
    void flush_batch() {
      if (batch_state.instances.empty()) {
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
      // Use identity matrix for UI batches to avoid camera transformation
      if (batch_state.is_ui_batch) {
        active_shader->set_mat4("u_view", glm::mat4(1.0f));
      }
      else {
        active_shader->set_mat4("u_view", get_view_matrix());
      }
      active_shader->set_mat4("u_projection", get_projection_matrix());

      // If using distortion shader, set distortion uniforms
      if (active_shader == distortion_shader.get()) {
        active_shader->set_float("u_time", Engine::Time::get_time());
        active_shader->set_float("u_intensity", batch_state.distortion_intensity);
        active_shader->set_float("u_speed", batch_state.distortion_speed);
        active_shader->set_float("u_mesh_intensity", batch_state.distortion_mesh_intensity);
      }

      // Upload instance data to GPU
      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, batch_state.instances.size() * sizeof(InstanceData), batch_state.instances.data());

      // Draw all quads in one instanced call
      glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, static_cast<GLsizei>(batch_state.instances.size()));

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

    // Add an instance to the instanced batch
    void batch_instance(GLuint texture_id, const InstanceData& instance, bool is_ui = false, Shader* use_shader = nullptr, f32 intensity = 0.0f, f32 speed = 1.0f, f32 mesh_intensity = 0.0f) {
      // Flush if switching between UI and world rendering
      if (is_ui != batch_state.is_ui_batch && !batch_state.instances.empty()) {
        flush_batch();
      }

      // Check if we need to flush due to texture or shader change
      if ((batch_state.current_texture_id != 0 && batch_state.current_texture_id != texture_id) ||
        (batch_state.current_shader != use_shader)) {
        flush_batch();
      }

      // Check if we need to flush due to batch being full
      if (batch_state.instances.size() >= MAX_INSTANCE_COUNT) {
        flush_batch();
      }

      // Set batch properties
      batch_state.is_ui_batch = is_ui;
      batch_state.current_texture_id = texture_id;
      batch_state.current_shader = use_shader;
      batch_state.distortion_intensity = intensity;
      batch_state.distortion_speed = speed;
      batch_state.distortion_mesh_intensity = mesh_intensity;

      // Add instance to batch
      batch_state.instances.push_back(instance);
    }

    // Helper function to draw a quad with custom texture coordinates (instanced)
    void draw_quad(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size,
      f32 rotation, const Math::Vec4D& color, const std::array<Math::Vec2D, 4>& tex_coords, const Math::Vec2D& flip = Math::Vec2D(1.0f, 1.0f)) {
      if (!texture || texture->id == 0) {
        Log::warning("Attempted to draw invalid texture: {}", texture->path);
        return;
      }

      if (!shader || !shader->is_valid()) {
        Log::error("Instanced shader is not valid");
        return;
      }

      // Build instance data - GPU does the vertex transformation
      InstanceData inst;

      // Create model matrix (translate, rotate, scale)
      inst.model = glm::mat4(1.0f);
      inst.model = glm::translate(inst.model, glm::vec3(position.x, position.y, 0.0f));

      if (rotation != 0.0f) {
        inst.model = glm::rotate(inst.model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      }

      inst.model = glm::scale(inst.model, glm::vec3(size.x * flip.x, size.y * flip.y, 1.0f));

      inst.color = glm::vec4(color.x, color.y, color.z, color.w);

      // Extract UV bounds: tex_coords[3] = top-left (min), tex_coords[1] = bottom-right (max)
      inst.uv_min = glm::vec2(tex_coords[3].x, tex_coords[3].y);
      inst.uv_max = glm::vec2(tex_coords[1].x, tex_coords[1].y);

      // Add to instanced batch
      batch_instance(texture->id, inst);
    }

    // Helper function to draw a glyph (character) at a specific position (instanced)
    void draw_glyph(const TextureData& atlas_texture, const std::array<Math::Vec2D, 4>& tex_coords,
      const Math::Vec2D& position, const Math::Vec2D& size, const Math::Vec4D& color) {
      if (atlas_texture.id == 0) {
        return;
      }

      if (!shader || !shader->is_valid()) {
        return;
      }

      // Build instance data: glyph at (position.x, position.y) with given size
      // The glyph's top-left is at (position), size is (w, h)
      // Unit quad is centered, so translate to center of glyph
      InstanceData inst;
      inst.model = glm::mat4(1.0f);
      inst.model = glm::translate(inst.model, glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, 0.0f));
      inst.model = glm::scale(inst.model, glm::vec3(size.x, size.y, 1.0f));

      inst.color = glm::vec4(color.x, color.y, color.z, color.w);

      // Extract UV bounds from tex_coords
      inst.uv_min = glm::vec2(tex_coords[3].x, tex_coords[3].y);
      inst.uv_max = glm::vec2(tex_coords[1].x, tex_coords[1].y);

      // Add to instanced batch
      batch_instance(atlas_texture.id, inst);
    }

    // UI-specific helper functions (no camera transformation)

    // Helper function to draw a quad in UI space (instanced, no view matrix applied)
    void draw_ui_quad(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size,
      f32 rotation, const Math::Vec4D& color, const std::array<Math::Vec2D, 4>& tex_coords, const Math::Vec2D& flip = Math::Vec2D(1.0f, 1.0f)) {
      if (!texture || texture->id == 0) {
        Log::warning("Attempted to draw invalid texture: {}", texture->path);
        return;
      }

      if (!shader || !shader->is_valid()) {
        Log::error("Instanced shader is not valid");
        return;
      }

      // Build instance data - GPU does the vertex transformation
      InstanceData inst;
      inst.model = glm::mat4(1.0f);
      inst.model = glm::translate(inst.model, glm::vec3(position.x, position.y, 0.0f));

      if (rotation != 0.0f) {
        inst.model = glm::rotate(inst.model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      }

      inst.model = glm::scale(inst.model, glm::vec3(size.x * flip.x, size.y * flip.y, 1.0f));

      inst.color = glm::vec4(color.x, color.y, color.z, color.w);
      inst.uv_min = glm::vec2(tex_coords[3].x, tex_coords[3].y);
      inst.uv_max = glm::vec2(tex_coords[1].x, tex_coords[1].y);

      // Add to instanced batch with UI flag (identity view matrix)
      batch_instance(texture->id, inst, true);
    }

    // Helper function to draw a UI glyph (instanced, no camera transformation)
    void draw_ui_glyph(const TextureData& atlas_texture, const std::array<Math::Vec2D, 4>& tex_coords,
      const Math::Vec2D& position, const Math::Vec2D& size, const Math::Vec4D& color) {
      if (atlas_texture.id == 0) {
        return;
      }

      if (!shader || !shader->is_valid()) {
        return;
      }

      // Build instance data: glyph at (position.x, position.y) with given size
      InstanceData inst;
      inst.model = glm::mat4(1.0f);
      inst.model = glm::translate(inst.model, glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, 0.0f));
      inst.model = glm::scale(inst.model, glm::vec3(size.x, size.y, 1.0f));

      inst.color = glm::vec4(color.x, color.y, color.z, color.w);
      inst.uv_min = glm::vec2(tex_coords[3].x, tex_coords[3].y);
      inst.uv_max = glm::vec2(tex_coords[1].x, tex_coords[1].y);

      // Add to instanced batch with UI flag
      batch_instance(atlas_texture.id, inst, true);
    }


    bool init() {
      // Load instanced shader (pairs instanced vertex shader with default fragment shader)
      shader = Shader::load_from_file("Assets/shaders/instanced.vert", "Assets/shaders/default.frag");

      if (!shader->is_valid()) {
        Log::error("Failed to create instanced shader");
        return false;
      }

      // Load instanced distortion shader
      distortion_shader = Shader::load_from_file("Assets/shaders/instanced_distortion.vert", "Assets/shaders/distortion.frag");

      if (!distortion_shader->is_valid()) {
        Log::error("Failed to create instanced distortion shader");
        return false;
      }

      // === Instanced Rendering VAO Setup ===
      glGenVertexArrays(1, &vao);
      glGenBuffers(1, &quad_vbo);
      glGenBuffers(1, &ebo);
      glGenBuffers(1, &instance_vbo);

      glBindVertexArray(vao);

      // Static unit quad geometry (shared by ALL instances - only 4 vertices)
      float quad_vertices[] = {
        // position      // texCoord
        -0.5f,  0.5f,    0.0f, 1.0f,   // Bottom-left
         0.5f,  0.5f,    1.0f, 1.0f,   // Bottom-right
         0.5f, -0.5f,    1.0f, 0.0f,   // Top-right
        -0.5f, -0.5f,    0.0f, 0.0f,   // Top-left
      };

      GLuint quad_indices[] = { 0, 1, 2, 2, 3, 0 };

      // Quad VBO (static - never changes)
      glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

      // Per-vertex: position (location 0)
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

      // Per-vertex: texCoord (location 1)
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

      // Quad index buffer (static)
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);

      // Instance VBO (dynamic - updated every frame)
      glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * MAX_INSTANCE_COUNT, nullptr, GL_DYNAMIC_DRAW);

      // Per-instance: model matrix (mat4 = 4 x vec4, locations 2-5)
      for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(2 + i);
        glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
          (void*)(offsetof(InstanceData, model) + sizeof(glm::vec4) * i));
        glVertexAttribDivisor(2 + i, 1); // Advance once per INSTANCE, not per vertex
      }

      // Per-instance: color (location 6)
      glEnableVertexAttribArray(6);
      glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
        (void*)offsetof(InstanceData, color));
      glVertexAttribDivisor(6, 1);

      // Per-instance: UV min (location 7)
      glEnableVertexAttribArray(7);
      glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
        (void*)offsetof(InstanceData, uv_min));
      glVertexAttribDivisor(7, 1);

      // Per-instance: UV max (location 8)
      glEnableVertexAttribArray(8);
      glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
        (void*)offsetof(InstanceData, uv_max));
      glVertexAttribDivisor(8, 1);

      glBindVertexArray(0);

      // === Polygon Rendering Setup (for arbitrary triangle geometry) ===
      polygon_shader = Shader::load_from_file("Assets/shaders/default.vert", "Assets/shaders/default.frag");
      if (!polygon_shader->is_valid()) {
        Log::error("Failed to create polygon shader");
        return false;
      }

      glGenVertexArrays(1, &polygon_vao);
      glGenBuffers(1, &polygon_vbo);

      glBindVertexArray(polygon_vao);
      glBindBuffer(GL_ARRAY_BUFFER, polygon_vbo);
      // Dynamic buffer for polygon triangle vertices (position 2f + texcoord 2f + color 4f = 32 bytes/vert)
      glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8 * 1024, nullptr, GL_DYNAMIC_DRAW);

      // Position (location 0)
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
      // TexCoord (location 1)
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
      // Color (location 2)
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));

      glBindVertexArray(0);

      // Setup OpenGL state
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      // Initialize camera to default
      reset_camera();

      // Reserve space for instanced batch
      batch_state.instances.reserve(MAX_INSTANCE_COUNT);

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

      Log::info("Renderer initialized with instanced rendering (max {} instances per batch)", MAX_INSTANCE_COUNT);
      Log::info("Virtual resolution: {}x{}", (int)virtual_width, (int)virtual_height);
      return true;
    }

    bool exit() {
      // Delete GPU resources
      if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
      }
      if (quad_vbo != 0) {
        glDeleteBuffers(1, &quad_vbo);
        quad_vbo = 0;
      }
      if (ebo != 0) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
      }
      if (instance_vbo != 0) {
        glDeleteBuffers(1, &instance_vbo);
        instance_vbo = 0;
      }

      // Delete shaders
      shader.reset();
      distortion_shader.reset();
      polygon_shader.reset();

      // Delete polygon rendering resources
      if (polygon_vao != 0) {
        glDeleteVertexArrays(1, &polygon_vao);
        polygon_vao = 0;
      }
      if (polygon_vbo != 0) {
        glDeleteBuffers(1, &polygon_vbo);
        polygon_vbo = 0;
      }

      // Clean up unit_rect texture
      unit_rect.reset();

      Log::info("Renderer cleaned up");
      return true;
    }

    void set_virtual_resolution(f32 width, f32 height) {
      virtual_width = width;
      virtual_height = height;
      Log::info("Virtual resolution set to: {}x{}", (int)width, (int)height);
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

    Math::Vec2D screen_to_viewport(const Math::Vec2D& screen_pos) {

#ifndef GAME_BUILD
      if (!Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().panel_viewport_mode) {
        // This means that panel viewport mode is disabled, so we can just return the virtual coordinates
        return screen_to_virtual(screen_pos);
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

      // Convert to virtual coordinates
      Math::Vec2D virtual_pos;
      virtual_pos.x = normalized_x * virtual_width;
      virtual_pos.y = normalized_y * virtual_height;

      return virtual_pos;
#else
      return screen_to_virtual(screen_pos);
#endif
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
      // Draw line as a thin rotated quad using instanced rendering
      f32 dx = end.x - start.x;
      f32 dy = end.y - start.y;
      f32 length = std::sqrt(dx * dx + dy * dy);
      if (length < 0.0001f) return;

      // Midpoint of the line segment
      f32 mid_x = (start.x + end.x) * 0.5f;
      f32 mid_y = (start.y + end.y) * 0.5f;

      // Angle from start to end
      f32 angle = std::atan2(dy, dx);

      // Build instance: a unit quad scaled to (length x thickness), rotated, centered at midpoint
      InstanceData inst;
      inst.model = glm::mat4(1.0f);
      inst.model = glm::translate(inst.model, glm::vec3(mid_x, mid_y, 0.0f));
      inst.model = glm::rotate(inst.model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
      inst.model = glm::scale(inst.model, glm::vec3(length, thickness, 1.0f));

      inst.color = glm::vec4(color.x, color.y, color.z, color.w);
      inst.uv_min = glm::vec2(0.0f, 0.0f);
      inst.uv_max = glm::vec2(1.0f, 1.0f);

      // Use unit_rect (1x1 white pixel) for solid color
      batch_instance(unit_rect->id, inst, false);
    }

    void draw_polygon(const std::vector<Math::Vec2D>& vertices, const Math::Vec2D& position, f32 rotation, const Math::Vec4D& color) {
      if (vertices.size() < 3) {
        Log::warning("Attempted to draw polygon with less than 3 vertices");
        return;
      }

      // Flush instanced batch to maintain draw order
      flush_batch();

      // Convert rotation to radians
      f32 angle_rad = rotation * 3.14159265359f / 180.0f;
      f32 cos_angle = std::cos(angle_rad);
      f32 sin_angle = std::sin(angle_rad);

      // Transform and rotate vertices
      std::vector<Math::Vec2D> transformed_vertices;
      transformed_vertices.reserve(vertices.size());

      for (const auto& vertex : vertices) {
        f32 rotated_x = vertex.x * cos_angle - vertex.y * sin_angle;
        f32 rotated_y = vertex.x * sin_angle + vertex.y * cos_angle;
        transformed_vertices.push_back(Math::Vec2D(rotated_x + position.x, rotated_y + position.y));
      }

      // Get the unit rect texture for solid color fill
      auto texture = Engine::Resources::get_rect_texture();
      if (!texture || texture->id == 0) return;

      // Calculate centroid for fan triangulation
      Math::Vec2D centroid(0.0f, 0.0f);
      for (const auto& v : transformed_vertices) {
        centroid = centroid + v;
      }
      centroid = centroid * (1.0f / transformed_vertices.size());

      // Build triangle fan vertex data: position(2f) + texcoord(2f) + color(4f) = 8 floats per vertex
      glm::vec4 col(color.x, color.y, color.z, color.w);
      std::vector<float> tri_data;
      tri_data.reserve(transformed_vertices.size() * 3 * 8);

      for (size_t i = 0; i < transformed_vertices.size(); ++i) {
        size_t next_i = (i + 1) % transformed_vertices.size();
        Math::Vec2D v0 = centroid;
        Math::Vec2D v1 = transformed_vertices[i];
        Math::Vec2D v2 = transformed_vertices[next_i];

        // Triangle vertex 0 (centroid)
        tri_data.insert(tri_data.end(), { v0.x, v0.y, 0.5f, 0.5f, col.r, col.g, col.b, col.a });
        // Triangle vertex 1
        tri_data.insert(tri_data.end(), { v1.x, v1.y, 0.5f, 0.5f, col.r, col.g, col.b, col.a });
        // Triangle vertex 2
        tri_data.insert(tri_data.end(), { v2.x, v2.y, 0.5f, 0.5f, col.r, col.g, col.b, col.a });
      }

      // Draw using polygon shader + direct GL_TRIANGLES
      polygon_shader->use();
      polygon_shader->set_mat4("u_projection", get_projection_matrix());
      polygon_shader->set_mat4("u_view", get_view_matrix());
      polygon_shader->set_int("u_texture", 0);

      glBindTexture(GL_TEXTURE_2D, texture->id);
      glBindVertexArray(polygon_vao);
      glBindBuffer(GL_ARRAY_BUFFER, polygon_vbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, tri_data.size() * sizeof(float), tri_data.data());
      glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(tri_data.size() / 8));
      glBindVertexArray(0);
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

    void draw_texture(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, const Math::Vec2D& flip) {
      // Default texture coordinates for full texture (bottom-left, bottom-right, top-right, top-left)
      std::array<Math::Vec2D, 4> default_tex_coords = {
        Math::Vec2D(0.0f, 1.0f), // Bottom-left
        Math::Vec2D(1.0f, 1.0f), // Bottom-right
        Math::Vec2D(1.0f, 0.0f), // Top-right
        Math::Vec2D(0.0f, 0.0f)  // Top-left
      };

      draw_quad(texture, position, size, rotation, color, default_tex_coords, flip);
    }

    void draw_subtexture(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords,
      const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, const Math::Vec2D& flip) {
      draw_quad(texture, position, size, rotation, color, tex_coords, flip);
    }

    void draw_subtexture_tiled(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords,
      const Math::Vec2D& position, const Math::Vec2D& size, const Math::Vec2D& tile_size, f32 rotation, const Math::Vec4D& color, const Math::Vec2D& flip) {

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
          draw_quad(texture, Math::Vec2D(tile_x, tile_y), tile_size, rotation, color, tex_coords, flip);
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

    // Helper function to split text into words
    static std::vector<std::string> split_into_words(const std::string& text) {
      std::vector<std::string> words;
      std::string current_word;

      for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (c == ' ' || c == '\n' || c == '\t') {
          if (!current_word.empty()) {
            words.push_back(current_word);
            current_word.clear();
          }
          // Add newline as a special "word" to force line break
          if (c == '\n') {
            words.push_back("\n");
          }
        }
        else {
          current_word += c;
        }
      }

      // Don't forget the last word
      if (!current_word.empty()) {
        words.push_back(current_word);
      }

      return words;
    }

    // Helper function to wrap text into lines based on max_width
    static std::vector<std::string> wrap_text_into_lines(Engine::Graphics::Font font, const std::string& text, f32 scale, f32 max_width) {
      std::vector<std::string> lines;

      if (!font || max_width <= 0.0f) {
        // No wrapping, return single line
        lines.push_back(text);
        return lines;
      }

      std::vector<std::string> words = split_into_words(text);
      std::string current_line;

      // Measure space width
      f32 space_width = 0.0f;
      auto space_it = font->glyphs.find(' ');
      if (space_it != font->glyphs.end()) {
        space_width = (space_it->second.advance >> 6) * scale;
      }

      for (const auto& word : words) {
        // Handle explicit newlines
        if (word == "\n") {
          lines.push_back(current_line);
          current_line.clear();
          continue;
        }

        // Measure the word
        f32 word_width = measure_text(font, word, scale).x;

        if (current_line.empty()) {
          // First word on the line
          current_line = word;
        }
        else {
          // Check if adding this word would exceed max_width
          f32 line_width = measure_text(font, current_line, scale).x;
          f32 potential_width = line_width + space_width + word_width;

          if (potential_width <= max_width) {
            // Word fits, add it
            current_line += " " + word;
          }
          else {
            // Word doesn't fit, start new line
            lines.push_back(current_line);
            current_line = word;
          }
        }
      }

      // Don't forget the last line
      if (!current_line.empty()) {
        lines.push_back(current_line);
      }

      // Handle empty text
      if (lines.empty()) {
        lines.push_back("");
      }

      return lines;
    }

    void draw_text_wrapped(Engine::Graphics::Font font, const std::string& text, const Math::Vec2D& position, f32 scale, const Math::Vec4D& color, f32 max_width, f32 line_spacing, int alignment) {
      if (!font || font->texture_atlas.id == 0) {
        Log::warning("Attempted to draw wrapped text with invalid font");
        return;
      }

      if (text.empty()) {
        return;
      }

      // If max_width is 0 or negative, just use regular draw_text
      if (max_width <= 0.0f) {
        draw_text(font, text, position, scale, color);
        return;
      }

      // Wrap text into lines
      std::vector<std::string> lines = wrap_text_into_lines(font, text, scale, max_width);

      // Calculate line height
      f32 line_height = font->baseline * scale * line_spacing;

      // Draw each line
      f32 y = position.y;
      for (const auto& line : lines) {
        f32 x = position.x;

        // Apply alignment
        if (alignment != 0 && !line.empty()) {
          f32 line_width = measure_text(font, line, scale).x;
          if (alignment == 1) {
            // Center alignment
            x = position.x + (max_width - line_width) * 0.5f;
          }
          else if (alignment == 2) {
            // Right alignment
            x = position.x + max_width - line_width;
          }
        }

        draw_text(font, line, Math::Vec2D(x, y), scale, color);
        y += line_height;
      }
    }

    Math::Vec2D measure_text_wrapped(Engine::Graphics::Font font, const std::string& text, f32 scale, f32 max_width) {
      if (!font || font->texture_atlas.id == 0) {
        Log::warning("Attempted to measure wrapped text with invalid font");
        return Math::Vec2D(0.0f, 0.0f);
      }

      if (text.empty()) {
        return Math::Vec2D(0.0f, 0.0f);
      }

      // If max_width is 0 or negative, just use regular measure_text
      if (max_width <= 0.0f) {
        return measure_text(font, text, scale);
      }

      // Wrap text into lines
      std::vector<std::string> lines = wrap_text_into_lines(font, text, scale, max_width);

      // Calculate total dimensions
      f32 total_width = 0.0f;
      f32 line_height = font->baseline * scale;

      for (const auto& line : lines) {
        f32 line_width = measure_text(font, line, scale).x;
        if (line_width > total_width) {
          total_width = line_width;
        }
      }

      f32 total_height = line_height * static_cast<f32>(lines.size());

      return Math::Vec2D(total_width, total_height);
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
    // Helper function to draw a quad with distortion effect (instanced)
    void draw_quad_distortion(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size,
      f32 rotation, const Math::Vec4D& color, const std::array<Math::Vec2D, 4>& tex_coords,
      f32 intensity, f32 speed, f32 mesh_intensity, const Math::Vec2D& flip = Math::Vec2D(1.0f, 1.0f)) {
      if (!texture || texture->id == 0) {
        Log::warning("Attempted to draw invalid texture");
        return;
      }

      if (!distortion_shader || !distortion_shader->is_valid()) {
        Log::error("Instanced distortion shader is not valid");
        return;
      }

      // Build instance data - GPU does vertex transformation + distortion
      InstanceData inst;
      inst.model = glm::mat4(1.0f);
      inst.model = glm::translate(inst.model, glm::vec3(position.x, position.y, 0.0f));

      if (rotation != 0.0f) {
        inst.model = glm::rotate(inst.model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      }

      inst.model = glm::scale(inst.model, glm::vec3(size.x * flip.x, size.y * flip.y, 1.0f));

      inst.color = glm::vec4(color.x, color.y, color.z, color.w);
      inst.uv_min = glm::vec2(tex_coords[3].x, tex_coords[3].y);
      inst.uv_max = glm::vec2(tex_coords[1].x, tex_coords[1].y);

      // Add to instanced batch with distortion shader
      batch_instance(texture->id, inst, false, distortion_shader.get(), intensity, speed, mesh_intensity);
    }

    void draw_texture_distortion(Engine::Graphics::Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, f32 intensity, f32 speed, f32 mesh_intensity, const Math::Vec2D& flip) {
      // Default texture coordinates (full texture)
      std::array<Math::Vec2D, 4> tex_coords = {
        Math::Vec2D(0.0f, 1.0f), // Bottom-left
        Math::Vec2D(1.0f, 1.0f), // Bottom-right
        Math::Vec2D(1.0f, 0.0f), // Top-right
        Math::Vec2D(0.0f, 0.0f)  // Top-left
      };

      draw_quad_distortion(texture, position, size, rotation, color, tex_coords, intensity, speed, mesh_intensity, flip);
    }

    void draw_subtexture_distortion(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, f32 intensity, f32 speed, f32 mesh_intensity, const Math::Vec2D& flip) {
      draw_quad_distortion(texture, position, size, rotation, color, tex_coords, intensity, speed, mesh_intensity, flip);
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
      // Draw UI line as a thin rotated quad using instanced rendering (no camera)
      f32 dx = end.x - start.x;
      f32 dy = end.y - start.y;
      f32 length = std::sqrt(dx * dx + dy * dy);
      if (length < 0.0001f) return;

      // Midpoint of the line segment
      f32 mid_x = (start.x + end.x) * 0.5f;
      f32 mid_y = (start.y + end.y) * 0.5f;

      // Angle from start to end
      f32 angle = std::atan2(dy, dx);

      // Build instance: a unit quad scaled to (length x thickness), rotated, centered at midpoint
      InstanceData inst;
      inst.model = glm::mat4(1.0f);
      inst.model = glm::translate(inst.model, glm::vec3(mid_x, mid_y, 0.0f));
      inst.model = glm::rotate(inst.model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
      inst.model = glm::scale(inst.model, glm::vec3(length, thickness, 1.0f));

      inst.color = glm::vec4(color.x, color.y, color.z, color.w);
      inst.uv_min = glm::vec2(0.0f, 0.0f);
      inst.uv_max = glm::vec2(1.0f, 1.0f);

      // Use unit_rect (1x1 white pixel) for solid color, UI flag = true
      batch_instance(unit_rect->id, inst, true);
    }

    void draw_polygon_ui(const std::vector<Math::Vec2D>& vertices, const Math::Vec2D& position, f32 rotation, const Math::Vec4D& color) {
      if (vertices.size() < 3) {
        Log::warning("Attempted to draw UI polygon with less than 3 vertices");
        return;
      }

      // Flush instanced batch to maintain draw order
      flush_batch();

      // Convert rotation to radians
      f32 angle_rad = rotation * 3.14159265359f / 180.0f;
      f32 cos_angle = std::cos(angle_rad);
      f32 sin_angle = std::sin(angle_rad);

      // Transform and rotate vertices
      std::vector<Math::Vec2D> transformed_vertices;
      transformed_vertices.reserve(vertices.size());

      for (const auto& vertex : vertices) {
        f32 rotated_x = vertex.x * cos_angle - vertex.y * sin_angle;
        f32 rotated_y = vertex.x * sin_angle + vertex.y * cos_angle;
        transformed_vertices.push_back(Math::Vec2D(rotated_x + position.x, rotated_y + position.y));
      }

      // Get the unit rect texture for solid color fill
      auto texture = Engine::Resources::get_rect_texture();
      if (!texture || texture->id == 0) return;

      // Calculate centroid for fan triangulation
      Math::Vec2D centroid(0.0f, 0.0f);
      for (const auto& v : transformed_vertices) {
        centroid = centroid + v;
      }
      centroid = centroid * (1.0f / transformed_vertices.size());

      // Build triangle fan vertex data: position(2f) + texcoord(2f) + color(4f) = 8 floats per vertex
      glm::vec4 col(color.x, color.y, color.z, color.w);
      std::vector<float> tri_data;
      tri_data.reserve(transformed_vertices.size() * 3 * 8);

      for (size_t i = 0; i < transformed_vertices.size(); ++i) {
        size_t next_i = (i + 1) % transformed_vertices.size();
        Math::Vec2D v0 = centroid;
        Math::Vec2D v1 = transformed_vertices[i];
        Math::Vec2D v2 = transformed_vertices[next_i];

        tri_data.insert(tri_data.end(), { v0.x, v0.y, 0.5f, 0.5f, col.r, col.g, col.b, col.a });
        tri_data.insert(tri_data.end(), { v1.x, v1.y, 0.5f, 0.5f, col.r, col.g, col.b, col.a });
        tri_data.insert(tri_data.end(), { v2.x, v2.y, 0.5f, 0.5f, col.r, col.g, col.b, col.a });
      }

      // Draw using polygon shader + direct GL_TRIANGLES (UI: identity view matrix)
      polygon_shader->use();
      polygon_shader->set_mat4("u_projection", get_projection_matrix());
      polygon_shader->set_mat4("u_view", glm::mat4(1.0f));
      polygon_shader->set_int("u_texture", 0);

      glBindTexture(GL_TEXTURE_2D, texture->id);
      glBindVertexArray(polygon_vao);
      glBindBuffer(GL_ARRAY_BUFFER, polygon_vbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, tri_data.size() * sizeof(float), tri_data.data());
      glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(tri_data.size() / 8));
      glBindVertexArray(0);
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

    void draw_texture_ui(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, const Math::Vec2D& flip) {
      // Default texture coordinates for full texture
      std::array<Math::Vec2D, 4> default_tex_coords = {
        Math::Vec2D(0.0f, 1.0f), // Bottom-left
        Math::Vec2D(1.0f, 1.0f), // Bottom-right
        Math::Vec2D(1.0f, 0.0f), // Top-right
        Math::Vec2D(0.0f, 0.0f)  // Top-left
      };

      draw_ui_quad(texture, position, size, rotation, color, default_tex_coords, flip);
    }

    void draw_subtexture_ui(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords,
      const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, const Math::Vec2D& flip) {
      draw_ui_quad(texture, position, size, rotation, color, tex_coords, flip);
    }

    void draw_tilemap(const std::unordered_map<int64_t, Tile>& tileMap,
      const std::vector<AtlasEntry>& loaded_tilemaps,
      const Math::Vec2D& tileSize,
      const Math::Vec2D& gridOffset,
      const Math::Vec4D& color) {

      if (tileMap.empty() || loaded_tilemaps.empty()) {
        return;
      }

      // Get current time for animation
      float current_time = Engine::Time::get_time();

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
      // EXCEPT for animated atlases - those cannot be merged
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

          // Check if this is an animated atlas - don't merge animated tiles
          if (atlas_entry.animation_config.is_animated) {
            processed[rel_y][rel_x] = true;
            rectangles.push_back({ abs_x, abs_y, 1, 1, tile });
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

        int tile_index_to_draw = rect.tile.tile_index;

        // Handle animated tiles
        if (atlas_entry.animation_config.is_animated) {
          const auto& anim_config = atlas_entry.animation_config;
          int total_frames = anim_config.get_frame_count();
          float total_duration = anim_config.get_total_duration();

          // Calculate current frame
          float elapsed = current_time;
          float time_in_anim;

          if (anim_config.is_looping) {
            time_in_anim = fmod(elapsed, total_duration);
          }
          else {
            time_in_anim = std::min(elapsed, total_duration - 0.001f);
          }

          int current_frame = static_cast<int>(time_in_anim / anim_config.frame_duration);
          current_frame = std::clamp(current_frame, 0, total_frames - 1);

          // Calculate the tile index offset for animation
          // The texture is laid out as a 2D grid:
          // - Total sheet cols/rows are stored in sheet_cols/sheet_rows
          // - Tiles are indexed row by row from top-left
          //
          // For a base tile index within frame 0, we need to:
          // 1. Find the tile's local position within the prop (local_col, local_row)
          // 2. Find the frame's position in the animation grid (frame_col, frame_row)
          // 3. Calculate the actual tile position in the sheet

          int tiles_per_frame = anim_config.prop_cols * anim_config.prop_rows;
          int base_tile = rect.tile.tile_index % tiles_per_frame;

          // Local position within the prop
          int local_col = base_tile % anim_config.prop_cols;
          int local_row = base_tile / anim_config.prop_cols;

          // Frame position in the animation grid
          int frame_col = current_frame % anim_config.anim_cols;
          int frame_row = current_frame / anim_config.anim_cols;

          // Calculate sheet position using stored sheet dimensions (avoids integer division precision loss)
          int tile_col = frame_col * anim_config.prop_cols + local_col;
          int tile_row = frame_row * anim_config.prop_rows + local_row;

          tile_index_to_draw = tile_row * anim_config.sheet_cols + tile_col;
        }

        if (tile_index_to_draw >= static_cast<int>(atlas_entry.subTextures.size())) {
          continue;
        }

        const auto& subTexture = atlas_entry.subTextures[tile_index_to_draw];

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

    void draw_text_wrapped_ui(Engine::Graphics::Font font, const std::string& text, const Math::Vec2D& position, f32 scale, const Math::Vec4D& color, f32 max_width, f32 line_spacing, int alignment) {
      if (!font || font->texture_atlas.id == 0) {
        Log::warning("Attempted to draw wrapped UI text with invalid font");
        return;
      }

      if (text.empty()) {
        return;
      }

      // If max_width is 0 or negative, just use regular draw_text_ui
      if (max_width <= 0.0f) {
        draw_text_ui(font, text, position, scale, color);
        return;
      }

      // Wrap text into lines
      std::vector<std::string> lines = wrap_text_into_lines(font, text, scale, max_width);

      // Calculate line height
      f32 line_height = font->baseline * scale * line_spacing;

      // Draw each line
      f32 y = position.y;
      for (const auto& line : lines) {
        f32 x = position.x;

        // Apply alignment
        if (alignment != 0 && !line.empty()) {
          f32 line_width = measure_text(font, line, scale).x;
          if (alignment == 1) {
            // Center alignment
            x = position.x + (max_width - line_width) * 0.5f;
          }
          else if (alignment == 2) {
            // Right alignment
            x = position.x + max_width - line_width;
          }
        }

        draw_text_ui(font, line, Math::Vec2D(x, y), scale, color);
        y += line_height;
      }
    }

  } // namespace Graphics
} // namespace Engine
