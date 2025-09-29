#include "BasicComponents.h"
#include "Core.h"

namespace Components
{
  StaticSprite::StaticSprite(std::string texturePath, Graphics::MinifyingFilter minifyingFilter)
  {
    // Generate a resource name from the texture path
    std::string resourceName = "texture_" + texturePath;

    // Replace path separators with underscores to create a valid resource name
    std::replace(resourceName.begin(), resourceName.end(), '/', '_');
    std::replace(resourceName.begin(), resourceName.end(), '\\', '_');
    std::replace(resourceName.begin(), resourceName.end(), '.', '_');

    // Load texture using ResourceManager
    Engine::RESOURCES.load_texture(resourceName, texturePath);
    textureId = Engine::RESOURCES.get_texture(resourceName);
  }

  StaticSprite::StaticSprite(const StaticSprite& other) : textureId(other.textureId) {}
}