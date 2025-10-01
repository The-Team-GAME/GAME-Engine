#include "ObjectFactory.h"
#include "Logger.h"
#include "BasicComponents.h"
#include "ECS/World.h"
#include "FileHandler.h"
#include "Core.h"

namespace fh = FileHandler;

Engine::ObjectFactory::ObjectFactory(ECS::World* world) : m_world(world)
{
}

int Engine::ObjectFactory::registerObject(std::string filePath)
{
  fh::set_container("entity");
  fh::parse_file(filePath);
  ObjectPrefab newObject;

  // TODO: check if data contains in debug mode only

  if (fh::has_key("id"))
  {
    if (fh::get_string("id") != fh::d_string())
    {
      newObject.objectId = Components::ObjectId(fh::get_string("id").c_str());
    }
    else { Log::warning("Key: id does not contain a value."); }
  }
  else { Log::warning("Key: id does not exist."); }


  if (fh::has_key("width") && fh::has_key("height"))
  {
    if (fh::get_f32("width") != fh::d_f32()
      && fh::get_f32("height") != fh::d_f32())
    {
      newObject.size = Components::Size(fh::get_f32("width"), fh::get_f32("height"));
    }
    else { Log::warning("Key: width/height does not contain a value."); }
  }
  else { Log::warning("Key: width/height does not exist."); }


  if (fh::has_key("texturePath"))
  {
    std::string texturePath = fh::get_string("texturePath");
    if (texturePath != fh::d_string())
    {
      // Generate a resource name from the texture path
      std::string resourceName = "texture_" + texturePath;
      std::replace(resourceName.begin(), resourceName.end(), '/', '_');
      std::replace(resourceName.begin(), resourceName.end(), '\\', '_');
      std::replace(resourceName.begin(), resourceName.end(), '.', '_');

      // Determine minifying filter
      Graphics::MinifyingFilter filter = Graphics::MinifyingFilter::Linear;
      if (fh::has_key("minifyingFilter") && fh::get_s32("minifyingFilter") != fh::d_s32())
      {
        filter = static_cast<Graphics::MinifyingFilter>(fh::get_s32("minifyingFilter"));
      }

      // Load texture and store the ID
      s32 textureId = Engine::RESOURCES.load_texture(resourceName, texturePath, filter);
      newObject.staticSprite = Components::StaticSprite(textureId);
    }
    else { Log::warning("Key: texturePath does not contain a value."); }
  }
  else { Log::warning("Key: texturePath does not exist."); }

  // Check for animated sprite (sprite sheet)
  if (fh::has_key("spriteSheetPath"))
  {
    std::string spriteSheetPath = fh::get_string("spriteSheetPath");
    if (spriteSheetPath != fh::d_string()
      && fh::has_key("frameWidth") && fh::has_key("frameHeight")
      && fh::has_key("totalFrames") && fh::has_key("framesPerSecond"))
    {
      // Generate a resource name from the sprite sheet path
      std::string resourceName = "spritesheet_" + spriteSheetPath;
      std::replace(resourceName.begin(), resourceName.end(), '/', '_');
      std::replace(resourceName.begin(), resourceName.end(), '\\', '_');
      std::replace(resourceName.begin(), resourceName.end(), '.', '_');

      s32 frameWidth = fh::get_s32("frameWidth");
      s32 frameHeight = fh::get_s32("frameHeight");
      s32 totalFrames = fh::get_s32("totalFrames");
      f32 framesPerSecond = fh::get_f32("framesPerSecond");

      // Determine minifying filter
      Graphics::MinifyingFilter filter = Graphics::MinifyingFilter::Linear;
      if (fh::has_key("minifyingFilter") && fh::get_s32("minifyingFilter") != fh::d_s32())
      {
        filter = static_cast<Graphics::MinifyingFilter>(fh::get_s32("minifyingFilter"));
      }

      // Load sprite sheet and store the ID
      s32 spriteSheetId = Engine::RESOURCES.load_sprite_sheet(
        resourceName, spriteSheetPath,
        frameWidth, frameHeight, totalFrames, framesPerSecond,
        filter
      );
      newObject.animatedSprite = Components::AnimatedSprite(spriteSheetId);
    }
    else
    {
      Log::warning("Sprite sheet requires: spriteSheetPath, frameWidth, frameHeight, totalFrames, framesPerSecond");
    }
  }

  objectList.push_back(newObject);
  s32 id = nextObjectId;
  nextObjectId++;

  fh::clear_json();

  return id;
}

void Engine::ObjectFactory::unregisterObject(int id)
{
  if (id < 0 || id >= static_cast<int>(objectList.size())) {
    return; // Invalid ID
  }
  objectList.erase(objectList.begin() + id);
  nextObjectId--;
}

void Engine::ObjectFactory::unregisterAllObjects()
{
  objectList.clear();
  nextObjectId = 0;
}

ECS::Entity Engine::ObjectFactory::createObject(int id)
{
  ObjectPrefab& objectPrefab = objectList[id];

  ECS::Entity entity = m_world->createEntity()
    .addComponent<Components::ObjectId>(objectPrefab.objectId)
    .addComponent<Components::Size>(objectPrefab.size);

  if (objectPrefab.staticSprite.has_value()) {
    entity.addComponent<Components::StaticSprite>(objectPrefab.staticSprite.value());
  }

  if (objectPrefab.animatedSprite.has_value()) {
    entity.addComponent<Components::AnimatedSprite>(objectPrefab.animatedSprite.value());
  }

  return entity;
}









// This was code that was previously written, but objects should not have attributes such as
// velocity, size, rotation, etc. by default. They should be added as needed after creation.
// Keeping this here for when we load level data where we want to load the above attributes.
//int Engine::ObjectFactory::registerObject(std::string filePath)
//{
//  std::ifstream f(filePath);
//  json data = json::parse(f);
//
//  ObjectPrefab newObject;
//
//  if (data.contains("position")) {
//    newObject.position = Components::Position(data["position"]["x"], data["position"]["y"]);
//  }
//
//  if (data.contains("velocity")) {
//    newObject.velocity = Components::Velocity(data["velocity"]["x"], data["velocity"]["y"]);
//  }
//
//  if (data.contains("size")) {
//    newObject.size = Components::Size(data["size"]["width"], data["size"]["height"]);
//  }
//
//  if (data.contains("rotation")) {
//    newObject.rotation = Components::Rotation(data["rotation"]);
//  }
//
//  if (data.contains("staticSprite")) {
//    if (data["staticSprite"].contains("minifyingFilter")) {
//      std::string filterStr = data["staticSprite"]["minifyingFilter"];
//
//      Graphics::MinifyingFilter filter = Graphics::MinifyingFilter::Linear;
//      if (filterStr == "Nearest") {
//        filter = Graphics::MinifyingFilter::Nearest;
//      }
//
//      newObject.staticSprite = Components::StaticSprite(data["staticSprite"]["texturePath"], filter);
//    }
//    else {
//      newObject.staticSprite = Components::StaticSprite(data["staticSprite"]["texturePath"]);
//    }
//  }
//
//  objectList.push_back(newObject);
//  s32 id = nextObjectId;
//  nextObjectId++;
//
//  return id;
//}