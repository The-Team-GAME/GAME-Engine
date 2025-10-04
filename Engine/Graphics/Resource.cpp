#include "Resource.h"


bool Engine::Resources::init()
{

}

Engine::Graphics::Texture Engine::Resources::load_texture(const std::string& filepath)
{
  return Engine::Graphics::Texture();
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
