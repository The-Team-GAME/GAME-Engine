#include "Renderer.h"
#include "../Logger.h"

bool Engine::Graphics::init()
{
  // Load shaders

  // Setup OpenGL state

  // Init FreeType

  if (FT_Init_FreeType(&ft))
  {
    Log::warning("Could not initialize FreeType library");
    return false;
  }


  return true;
}

bool Engine::Graphics::exit()
{
  return true;
}

void Engine::Graphics::draw_texture(Texture texture, const Math::Vec2D& position, const Math::Vec2D& size, f32 rotation, const Math::Vec4D& color)
{
}
