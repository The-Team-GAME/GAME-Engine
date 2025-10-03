//-----------------------------------------------------------------------------------
// file:    Graphics.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the Graphics class which provides a comprehensive 2D
//          rendering system. Features include batch rendering, texture management,
//          sprite sheets, animated sprites, text rendering with FreeType, camera
//          controls (position, zoom, rotation), and primitive shape drawing.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Precompiled.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <array>
#include "Shader.h"
#include "TypeDef.h"

class Graphics
{
private:
  s32 win_width = 0;
  s32 win_height = 0;
public:
  // Add these for consistent virtual resolution
  static constexpr f32 VIRTUAL_WIDTH = 1600.0f;
  static constexpr f32 VIRTUAL_HEIGHT = 900.0f;

  /* =================== //
  // Graphics structures //
  // =================== */
  struct Texture
  {
    u32 id;
    s32 width;
    s32 height;
  };
  struct Camera2D
  {
    glm::vec2 position = { 0.0f, 0.0f };
    f32 zoom = 1.0f;
    f32 rotation = 0.0f;
  };

  /* ================= //
  // Graphics enums    //
  // ================= */
  enum class MinifyingFilter
  {
    Nearest,
    Linear
  };

  enum class Origin
  {
    Center,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
  };

  /* ======================== //
  // Private structures       //
  // ======================== */
private:
  struct Vertex
  {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 tex_coords;
    f32 tex_id;
  };

  struct SpriteSheet
  {
    u32 texture_id;
    s32 frame_width;
    s32 frame_height;
    s32 total_frames;
    f32 animation_speed; // Frames per second
  };

  // Font rendering structures
  struct Character {
    glm::vec2 tex_coords[4];  // UV coordinates in the atlas
    glm::ivec2 size;
    glm::ivec2 bearing;
    u32 advance;
  };

  struct Font {
    std::unordered_map<char, Character> characters;
    u32 atlas_texture = 0;  // Single texture for all characters
    u32 font_size = 0;
    f32 baseline = 0.0f;
    s32 atlas_width = 0, atlas_height = 0;
    bool is_valid = true;  // Track if font is still valid
  };

  /* ========================== //
  // Batch rendering constants  //
  // ========================== */
  static const u32 MaxQuads = 10000;
  static const u32 MaxVertices = MaxQuads * 4;
  static const u32 MaxIndices = MaxQuads * 6;
  static const u32 MaxTextureSlots = 32; // Check your GPU's limit

  /* ======================= //
  // OpenGL buffers          //
  // ======================= */
  u32 quadVAO = 0;
  u32 quadVBO = 0;
  u32 quadIBO = 0;

  u32 quad_index_count = 0;
  std::vector<Vertex> quad_vertex_buffer;
  u32 vertex_index = 0;

  /* ========================= //
  // Resource storage          //
  // ========================= */
  std::unordered_map<s32, Texture> textures;
  std::unordered_map<s32, Font> fonts;
  std::unordered_map<s32, SpriteSheet> sprite_sheets;

  // ID counters for generating unique IDs
  s32 next_texture_id = 0;
  s32 next_font_id = 0;
  s32 next_sprite_sheet_id = 0;

  /* ========================= //
  // Texture binding slots     //
  // ========================= */
  std::array<u32, MaxTextureSlots> texture_slots = {};
  u32 texture_slot_index = 1; // 0 = white texture

  /* ========================= //
  // Rendering state           //
  // ========================= */
  Shader texture_shader{};
  glm::mat4 projection = glm::mat4(1.0f);
  glm::mat4 view = glm::mat4(1.0f);
  glm::mat4 view_projection_matrix = glm::mat4(1.0f);
  Camera2D camera;

  s32 viewport_width = 800;
  s32 viewport_height = 600;

  void update_view_matrix();

public:
  /* ===================================== //
  // Initialization and lifecycle          //
  // ===================================== */
  Graphics();
  void set_params(s32 win_width, s32 win_height);
  void init_graphics();
  ~Graphics();

  void begin_frame();
  void end_frame();

  void set_viewport(s32 width, s32 height);

  /* =================== //
  // Camera controls     //
  // =================== */
  void set_camera_position(f32 x, f32 y);
  void set_camera_zoom(f32 zoom);
  void set_camera_rotation(f32 degrees);

  /* ====================== //
  // Font and text rendering //
  // ====================== */
  s32 load_font(const char* filepath, u32 size);
  void unload_font(s32 fontId);
  void unload_all_fonts();
  void draw_text(s32 fontId, const std::string& text, f32 x, f32 y, f32 scale, const glm::vec4& color);

  /* ====================== //
  // Primitive drawing      //
  // ====================== */
  void draw_rect(const glm::vec4& color, f32 x, f32 y, f32 width, f32 height, f32 rotation = 0.0f, Origin origin = Origin::Center);
  void draw_circle(const glm::vec4& color, f32 x, f32 y, f32 radius, s32 segments = 32);
  void draw_line(const glm::vec4& color, f32 x1, f32 y1, f32 x2, f32 y2, f32 thickness = 2.0f);

  /* ====================== //
  // Texture drawing        //
  // ====================== */
  void draw_texture(s32 imageId, f32 x, f32 y, f32 width, f32 height, f32 rotation = 0.0f, Origin origin = Origin::Center, const glm::vec4& ts32Color = glm::vec4(1.0f));
  void draw_texture_region(s32 imageId, f32 destX, f32 destY, f32 destWidth, f32 destHeight, f32 srcX, f32 srcY, f32 srcWidth, f32 srcHeight, f32 rotation = 0.0f, Origin origin = Origin::Center, const glm::vec4& ts32Color = glm::vec4(1.0f));

  /* ======================== //
  // Texture management       //
  // ======================== */
  s32 load_texture(const char* filepath, MinifyingFilter min_filter = MinifyingFilter::Linear);
  void unload_texture(s32 texture_id);
  void unload_all_textures();
  void get_texture_dimensions(s32 texture_id, s32& width, s32& height) const;
  u32 get_texture_handle(s32 texture_id) const;

  /* ======================== //
  // Screen operations        //
  // ======================== */
  void clear_screen(f32 r, f32 g, f32 b, f32 a);
  void clear_screen(s32 texture_id);

  /* ============================ //
  // Sprite sheet management      //
  // ============================ */
  s32 load_sprite_sheet(const char* filepath, s32 frame_width, s32 frame_height, s32 total_frames, f32 animation_speed, MinifyingFilter min_filter = MinifyingFilter::Linear);
  void unload_sprite_sheet(s32 spriteSheetId);
  void unload_all_sprite_sheets();
  void draw_animated_sprite(s32 spriteSheetId, f32 x, f32 y, f32 width, f32 height, f32 time, f32 rotation = 0.0f, Origin origin = Origin::Center, const glm::vec4& ts32Color = glm::vec4(1.0f));

  struct SpriteSheetInfo {
    s32 frame_width;
    s32 frame_height;
    s32 total_frames;
    f32 animation_speed;
  };

  bool get_sprite_sheet_info(s32 spriteSheetId, SpriteSheetInfo& info) const;

  /* ======================== //
  // FreeType library         //
  // ======================== */
  FT_Library ft = {};

  /* ======================= //
  // Utility functions       //
  // ======================= */
  glm::vec2 screen_to_virtual(f32 screenX, f32 screenY) const;
  glm::vec2 screen_to_world(f32 screenX, f32 screenY) const;
  glm::vec2 world_to_screen(f32 worldX, f32 worldY) const;

private:
  /* ============================ //
  // Private rendering methods    //
  // ============================ */
  void start_batch();
  void next_batch();
  void flush();
  glm::vec2 calculate_origin_offset(f32 width, f32 height, Origin origin) const;
};