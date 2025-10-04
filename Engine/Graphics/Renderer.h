#pragma once

#include "../Vector2D.h"
#include "../Vector4D.h"
#include "RendererInternal.h"
#include <array>

namespace Engine {
  namespace Graphics {

    using Texture = std::shared_ptr<TextureData>;
    using Font = std::shared_ptr<FontData>;

    // Initialization code here (e.g., setting up OpenGL context, loading shaders)
    bool init();

    // Cleanup code here (e.g., deleting OpenGL resources)
    bool exit();

    void draw_texture(Engine::Graphics::Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color);

    // Draw a subtexture (part of a texture atlas) using custom texture coordinates
    // TODO: change to use Math::Vec2D for tex_coords
    void draw_subtexture(Engine::Graphics::Texture texture, const std::array<glm::vec2, 4>& tex_coords, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color);

  }
}
