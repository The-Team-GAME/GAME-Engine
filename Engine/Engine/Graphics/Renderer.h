//-----------------------------------------------------------------------------------
// file:    Renderer.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Public interface for the graphics rendering system. Defines camera
//          and viewport structures, texture/font types, and rendering functions.
//          Provides high-level drawing APIs for textures, sprites, text, and
//          geometric shapes with camera transformations and batching support.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Math/Vector2D.h" 
#include "../Math/Vector4D.h"
#include "RendererInternal.h"
#include <array>
#include <string>
#include <vector>

namespace Engine {
  namespace Graphics {

    using Texture = std::shared_ptr<TextureData>;
    using Font = std::shared_ptr<FontData>;

    // Camera structure for 2D rendering
    struct Camera {
      Math::Vec2D position = { 0.0f, 0.0f };  // Camera position in world space
      f32 rotation = 0.0f;                     // Camera rotation in degrees
      f32 zoom = 1.0f;                         // Camera zoom level (1.0 = normal, >1.0 = zoom in, <1.0 = zoom out)

      Camera() = default;
      Camera(const Math::Vec2D& pos, f32 rot = 0.0f, f32 z = 1.0f)
        : position(pos), rotation(rot), zoom(z) {
      }
    };

    // Axis-aligned bounding box for camera frustum culling
    struct CameraBounds {
      f32 min_x = 0.0f;
      f32 max_x = 0.0f;
      f32 min_y = 0.0f;
      f32 max_y = 0.0f;

      // Check if a point is inside the bounds
      bool contains(const Math::Vec2D& point) const {
        return point.x >= min_x && point.x <= max_x &&
               point.y >= min_y && point.y <= max_y;
      }

      // Check if an AABB intersects with these bounds
      bool intersects(f32 x, f32 y, f32 half_w, f32 half_h) const {
        return (x + half_w >= min_x) && (x - half_w <= max_x) &&
               (y + half_h >= min_y) && (y - half_h <= max_y);
      }
    };

    // Viewport structure
    struct Viewport {
      int x = 0;
      int y = 0;
      int width = 0;
      int height = 0;

      Viewport() = default;
      Viewport(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}
    };

    // Initialization code here (e.g., setting up OpenGL context, loading shaders)
    bool init();

    // Cleanup code here (e.g., deleting OpenGL resources)
    bool exit();

    // Begin a new batch (call at start of frame)
    void begin_batch();

    // End batch (call at end of frame)
    void end_batch();

    // Virtual resolution functions
    void set_virtual_resolution(f32 width, f32 height);
    Math::Vec2D get_virtual_resolution();

    // Camera functions
    void set_camera(const Camera& camera);
    Camera get_camera();
    void set_camera_position(const Math::Vec2D& position);
    void set_camera_rotation(f32 rotation);
    void set_camera_zoom(f32 zoom);
    void reset_camera();

    // Frustum culling functions
    // Get the visible world-space bounds of the current camera (AABB)
    CameraBounds get_camera_bounds();

    // Check if an axis-aligned bounding box is visible to the camera
    // position: center of the object in world space
    // size: full width and height of the object
    bool is_visible(const Math::Vec2D& position, const Math::Vec2D& size);

    // Check visibility with pre-computed camera bounds (more efficient for batch checks)
    bool is_visible(const CameraBounds& bounds, const Math::Vec2D& position, const Math::Vec2D& size);

    // Enable or disable automatic frustum culling in draw functions
    void set_frustum_culling(bool enabled);
    bool is_frustum_culling_enabled();

    // Viewport functions
    void set_viewport(const Viewport& viewport);
    Viewport get_viewport();

    // Coordinate conversion functions
    // Convert screen coordinates (actual window pixels) to virtual coordinates
    Math::Vec2D screen_to_virtual(const Math::Vec2D& screen_pos);

    // Convert screen coordinates to viewport-relative virtual coordinates (for ImGui viewport mode)
    Math::Vec2D screen_to_viewport(const Math::Vec2D& screen_pos);

    // Convert virtual coordinates to screen coordinates
    Math::Vec2D virtual_to_screen(const Math::Vec2D& virtual_pos);

    // Convert virtual coordinates to world coordinates (accounting for camera)
    Math::Vec2D virtual_to_world(const Math::Vec2D& virtual_pos);

    // Convert world coordinates to virtual coordinates (accounting for camera)
    Math::Vec2D world_to_virtual(const Math::Vec2D& world_pos);

    // Convert screen coordinates directly to world coordinates
    Math::Vec2D screen_to_world(const Math::Vec2D& screen_pos);

    // Convert world coordinates directly to screen coordinates
    Math::Vec2D world_to_screen(const Math::Vec2D& world_pos);

    // Drawing functions

    void draw_rect(const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color);

    void draw_rect_outline(const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, f32 thickness, const Math::Vec4D& color);

    void draw_line(const Math::Vec2D& start, const Math::Vec2D& end, f32 thickness, const Math::Vec4D& color);

    // Draw a polygon with specified vertices (filled)
    void draw_polygon(const std::vector<Math::Vec2D>& vertices, const Math::Vec2D& position, f32 rotation, const Math::Vec4D& color);

    // Draw a polygon outline with specified vertices
    void draw_polygon_outline(const std::vector<Math::Vec2D>& vertices, const Math::Vec2D& position, f32 rotation, f32 thickness, const Math::Vec4D& color);

    // Draw a triangle (filled)
    void draw_triangle(const Math::Vec2D& position, f32 radius, f32 rotation, const Math::Vec4D& color);

    // Draw a triangle outline
    void draw_triangle_outline(const Math::Vec2D& position, f32 radius, f32 rotation, f32 thickness, const Math::Vec4D& color);

    // Draw a circle (filled)
    void draw_circle(const Math::Vec2D& position, f32 radius, const Math::Vec4D& color, int segments = 32);

    // Draw a circle outline
    void draw_circle_outline(const Math::Vec2D& position, f32 radius, f32 thickness, const Math::Vec4D& color, int segments = 32);

    void draw_texture(Engine::Graphics::Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, const Math::Vec2D& flip = Math::Vec2D(1.0f, 1.0f));

    // Draw a subtexture (part of a texture atlas) using custom texture coordinates
    void draw_subtexture(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, const Math::Vec2D& flip = Math::Vec2D(1.0f, 1.0f));

    // Draw a tiled subtexture that repeats instead of scaling
    void draw_subtexture_tiled(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords, const Math::Vec2D& position, const Math::Vec2D& size, const Math::Vec2D& tile_size, f32 rotation, const Math::Vec4D& color, const Math::Vec2D& flip = Math::Vec2D(1.0f, 1.0f));

    // Draw tilemap from integer map with automatic rectangle merging for optimization
    // tileMap: Sparse hashmap of tile indices (key = encoded x,y coordinates)
    // atlas: Texture atlas containing all tile sprites
    // texture: The texture handle for the atlas
    // tileSize: Size of each tile in world units
    // gridOffset: World position offset for the tilemap origin
    // color: Tint color to apply to all tiles
    void draw_tilemap(const std::unordered_map<int64_t, Tile>& tileMap,
      const std::vector<AtlasEntry>& loaded_tilemaps,
      const Math::Vec2D& tileSize,
      const Math::Vec2D& gridOffset,
      const Math::Vec4D& color);

    // Draw texture with distortion effect
    void draw_texture_distortion(Engine::Graphics::Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, f32 intensity, f32 speed, f32 mesh_intensity = 0.0f, const Math::Vec2D& flip = Math::Vec2D(1.0f, 1.0f));

    // Draw subtexture with distortion effect
    void draw_subtexture_distortion(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, f32 intensity, f32 speed, f32 mesh_intensity = 0.0f, const Math::Vec2D& flip = Math::Vec2D(1.0f, 1.0f));

    // Draw text using a font
    void draw_text(Engine::Graphics::Font font, const std::string& text, const Math::Vec2D& position, f32 scale, const Math::Vec4D& color);

    // Draw text with word wrapping
    // alignment: 0 = left, 1 = center, 2 = right
    void draw_text_wrapped(Engine::Graphics::Font font, const std::string& text, const Math::Vec2D& position, f32 scale, const Math::Vec4D& color, f32 max_width, f32 line_spacing = 1.2f, int alignment = 0);

    // Measure text dimensions
    Math::Vec2D measure_text(Engine::Graphics::Font font, const std::string& text, f32 scale);

    // Measure wrapped text dimensions (returns bounding box of all lines)
    Math::Vec2D measure_text_wrapped(Engine::Graphics::Font font, const std::string& text, f32 scale, f32 max_width);

    // Clear the screen with a color or texture
    void clear_screen(const Math::Vec4D& color);
    void clear_screen(Engine::Graphics::Texture texture);

    // Get the unit rectangle texture (1x1 white pixel)
    Texture get_rect_texture();

    // UI Drawing functions (no camera transformation, drawn in screen space)

    void draw_rect_ui(const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color);

    void draw_rect_outline_ui(const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, f32 thickness, const Math::Vec4D& color);

    void draw_line_ui(const Math::Vec2D& start, const Math::Vec2D& end, f32 thickness, const Math::Vec4D& color);

    // Draw a UI polygon with specified vertices (filled)
    void draw_polygon_ui(const std::vector<Math::Vec2D>& vertices, const Math::Vec2D& position, f32 rotation, const Math::Vec4D& color);

    // Draw a UI polygon outline with specified vertices
    void draw_polygon_outline_ui(const std::vector<Math::Vec2D>& vertices, const Math::Vec2D& position, f32 rotation, f32 thickness, const Math::Vec4D& color);

    // Draw a UI triangle (filled)
    void draw_triangle_ui(const Math::Vec2D& position, f32 radius, f32 rotation, const Math::Vec4D& color);

    // Draw a UI triangle outline
    void draw_triangle_outline_ui(const Math::Vec2D& position, f32 radius, f32 rotation, f32 thickness, const Math::Vec4D& color);

    // Draw a UI circle (filled)
    void draw_circle_ui(const Math::Vec2D& position, f32 radius, const Math::Vec4D& color, int segments = 32);

    // Draw a UI circle outline
    void draw_circle_outline_ui(const Math::Vec2D& position, f32 radius, f32 thickness, const Math::Vec4D& color, int segments = 32);

    void draw_texture_ui(Engine::Graphics::Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, const Math::Vec2D& flip = Math::Vec2D(1.0f, 1.0f));

    // Draw a UI subtexture (part of a texture atlas) using custom texture coordinates
    void draw_subtexture_ui(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, const Math::Vec2D& flip = Math::Vec2D(1.0f, 1.0f));

    // Draw UI text using a font
    void draw_text_ui(Engine::Graphics::Font font, const std::string& text, const Math::Vec2D& position, f32 scale, const Math::Vec4D& color);

    // Draw UI text with word wrapping
    // alignment: 0 = left, 1 = center, 2 = right
    void draw_text_wrapped_ui(Engine::Graphics::Font font, const std::string& text, const Math::Vec2D& position, f32 scale, const Math::Vec4D& color, f32 max_width, f32 line_spacing = 1.2f, int alignment = 0);

  } // namespace Graphics
} // namespace Engine
