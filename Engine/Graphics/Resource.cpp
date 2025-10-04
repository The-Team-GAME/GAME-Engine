#include "Resource.h"
#include <memory>
#include "RendererInternal.h"

bool Engine::Resources::init()
{
  return true;
}

Engine::Graphics::Texture Engine::Resources::load_texture(const std::string& filepath)
{
  return std::make_shared<Engine::Graphics::TextureData>(filepath);
}

Engine::Graphics::Font Engine::Resources::load_font(const std::string& filepath, u32 size)
{
  return Engine::Graphics::Font();
}

Engine::Graphics::Texture Engine::Resources::get_unit_rect_texture()
{
  return Engine::Graphics::Texture();
}

void Engine::Resources::clear_unused_resources()
{
}
