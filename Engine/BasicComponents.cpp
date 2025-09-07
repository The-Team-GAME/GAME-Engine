#include "BasicComponents.h"
#include "Core.h"

namespace Components
{
  StaticSprite::StaticSprite(std::string texturePath, Graphics::MinifyingFilter minifyingFilter)
  {
    textureId = Engine::GRAPHICS->loadTexture(texturePath.c_str(), minifyingFilter);
  }

  StaticSprite::StaticSprite(const StaticSprite& other) : textureId(other.textureId) {}
}