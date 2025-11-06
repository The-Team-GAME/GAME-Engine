#pragma once

#include "../Math/Vector2D.h" 
#include "../Math/Vector4D.h"
#include "RendererInternal.h"
#include <array>
#include <string>

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

    // Viewport functions
    void set_viewport(const Viewport& viewport);
    Viewport get_viewport();

    // Coordinate conversion functions
    // Convert screen coordinates (actual window pixels) to virtual coordinates
    Math::Vec2D screen_to_virtual(const Math::Vec2D& screen_pos);

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

    void draw_texture(Engine::Graphics::Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color);

    // Draw a subtexture (part of a texture atlas) using custom texture coordinates
    void draw_subtexture(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color);

    // Draw texture with distortion effect
    void draw_texture_distortion(Engine::Graphics::Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, f32 intensity, f32 speed, f32 mesh_intensity = 0.0f);

    // Draw subtexture with distortion effect
    void draw_subtexture_distortion(Engine::Graphics::Texture texture, const std::array<Math::Vec2D, 4>& tex_coords, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color, f32 intensity, f32 speed, f32 mesh_intensity = 0.0f);

    // Draw text using a font
    void draw_text(Engine::Graphics::Font font, const std::string& text, const Math::Vec2D& position, f32 scale, const Math::Vec4D& color);

    // Measure text dimensions
    Math::Vec2D measure_text(Engine::Graphics::Font font, const std::string& text, f32 scale);

    // Clear the screen with a color or texture
    void clear_screen(const Math::Vec4D& color);
    void clear_screen(Engine::Graphics::Texture texture);

    // Get the unit rectangle texture (1x1 white pixel)
    Texture get_rect_texture();

  } // namespace Graphics
} // namespace Engine
