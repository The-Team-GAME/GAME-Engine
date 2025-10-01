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
    textureId = Engine::RESOURCES.load_texture(resourceName, texturePath, minifyingFilter);
  }

  StaticSprite::StaticSprite(const StaticSprite& other) : textureId(other.textureId) {}

  AnimatedSprite::AnimatedSprite(std::string spriteSheetPath, s32 frameWidth, s32 frameHeight,
                                 s32 totalFrames, f32 framesPerSecond,
                                 Graphics::MinifyingFilter minifyingFilter)
    : animationTime(0.0f), isPlaying(true), loop(true)
  {
    // Generate a resource name from the sprite sheet path
    std::string resourceName = "spritesheet_" + spriteSheetPath;

    // Replace path separators with underscores to create a valid resource name
    std::replace(resourceName.begin(), resourceName.end(), '/', '_');
    std::replace(resourceName.begin(), resourceName.end(), '\\', '_');
    std::replace(resourceName.begin(), resourceName.end(), '.', '_');

    // Load sprite sheet using ResourceManager
    spriteSheetId = Engine::RESOURCES.load_sprite_sheet(resourceName, spriteSheetPath,
                                                         frameWidth, frameHeight, totalFrames,
                                                         framesPerSecond, minifyingFilter);
  }

  AnimatedSprite::AnimatedSprite(const AnimatedSprite& other)
    : spriteSheetId(other.spriteSheetId), animationTime(other.animationTime),
      isPlaying(other.isPlaying), loop(other.loop) {}

  void AnimatedSprite::play() {
    isPlaying = true;
  }

  void AnimatedSprite::pause() {
    isPlaying = false;
  }

  void AnimatedSprite::stop() {
    isPlaying = false;
    animationTime = 0.0f;
  }

  void AnimatedSprite::reset() {
    animationTime = 0.0f;
  }
}