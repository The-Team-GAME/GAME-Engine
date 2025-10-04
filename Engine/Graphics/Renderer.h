#pragma once

#include "../Vector2D.h"
#include "../Vector4D.h"
#include "Types.h"

namespace Engine {
  namespace Graphics {

    // Initialization code here (e.g., setting up OpenGL context, loading shaders)
    bool init();

    // Cleanup code here (e.g., deleting OpenGL resources)
    bool exit();

    void draw_texture(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color);

  }
}
