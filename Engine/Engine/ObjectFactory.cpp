//TODO: deprecating soon

//-----------------------------------------------------------------------------------
// file:    ObjectFactory.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of the ObjectFactory namespace which manages object prefabs
//          and creates ECS entities from JSON configuration files.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ObjectFactory.h"
#include "Logger.h"
#include "Components/EngineComponents.h"
#include "FileHandler.h"
#include "Graphics/Renderer.h"
#include <algorithm>
#include "Resources.h"

namespace fh = FileHandler;

namespace Engine
{
  namespace ObjectFactory
  {
    // Internal state
    namespace
    {
      std::vector<ObjectPrefab> object_list;
      s32 next_object_id = 0;
    }

    s32 register_object(const std::string& filePath)
    {
      fh::set_container("entity");
      fh::parse_file(filePath);
      ObjectPrefab newObject;

      // Load object ID
      if (fh::has_key("id"))
      {
        if (fh::get_string("id") != fh::d_string())
        {
          newObject.object_id = Components::ObjectId(fh::get_string("id").c_str());
        }
        else { Log::warning("Key: id does not contain a value."); }
      }
      else { Log::warning("Key: id does not exist."); }

      // Load size (scale)
      if (fh::has_key("width") && fh::has_key("height"))
      {
        if (fh::get_f32("width") != fh::d_f32() && fh::get_f32("height") != fh::d_f32())
        {
          newObject.size = Math::Vec2D(fh::get_f32("width"), fh::get_f32("height"));
        }
        else { Log::warning("Key: width/height does not contain a value."); }
      }
      else { Log::warning("Key: width/height does not exist."); }

      // Load static texture
      if (fh::has_key("texturePath"))
      {
        std::string texturePath = fh::get_string("texturePath");
        if (texturePath != fh::d_string())
        {
          // Load texture using Resources system (caching is handled automatically)
          newObject.texture = Engine::Resources::load_texture(texturePath);

          if (!newObject.texture.has_value() || !newObject.texture.value() || newObject.texture.value()->id == 0)
          {
            Log::error("Failed to load texture: " + texturePath);
          }
        }
        else { Log::warning("Key: texturePath does not contain a value."); }
      }

      // Load sprite sheet (texture atlas + animation)
      if (fh::has_key("spriteSheetPath"))
      {
        std::string spriteSheetPath = fh::get_string("spriteSheetPath");
        if (spriteSheetPath != fh::d_string() &&
          fh::has_key("frameWidth") && fh::has_key("frameHeight") &&
          fh::has_key("totalFrames") && fh::has_key("framesPerSecond"))
        {
          s32 frame_width = fh::get_s32("frameWidth");
          s32 frame_height = fh::get_s32("frameHeight");
          s32 total_frames = fh::get_s32("totalFrames");
          f32 frames_per_second = fh::get_f32("framesPerSecond");

          // Load texture using Resources system (caching is handled automatically)
          newObject.texture = Engine::Resources::load_texture(spriteSheetPath);

          if (!newObject.texture.has_value() || !newObject.texture.value() || newObject.texture.value()->id == 0)
          {
            Log::error("Failed to load sprite sheet: " + spriteSheetPath);
          }
          else
          {
            // Calculate rows and columns based on texture dimensions
            // Need to dereference the shared_ptr to access width and height
            s32 cols = newObject.texture.value()->width / frame_width;
            s32 rows = newObject.texture.value()->height / frame_height;

            // Create texture atlas using the constructor
            // This will automatically generate the correct texture coordinates
            newObject.texture_atlas = Components::TextureAtlas(
              frame_width,    // subTextureWidth
              frame_height,   // subTextureHeight
              rows,           // rows
              cols,           // cols
              total_frames    // totalSubTextures (limits to total_frames if less than rows*cols)
            );

            // Create animation with evenly spaced frames
            f32 frame_duration = 1.0f / frames_per_second;
            newObject.animation = Components::Animation(total_frames, frame_duration);
          }
        }
        else
        {
          Log::warning("Sprite sheet requires: spriteSheetPath, frameWidth, frameHeight, totalFrames, framesPerSecond");
        }
      }

      object_list.push_back(newObject);
      s32 id = next_object_id;
      next_object_id++;

      fh::clear_json();

      return id;
    }

    void unregister_object(s32 id)
    {
      if (id < 0 || id >= static_cast<s32>(object_list.size())) {
        return;
      }
      object_list.erase(object_list.begin() + id);
      next_object_id--;
    }

    void unregister_all_objects()
    {
      object_list.clear();
      next_object_id = 0;
    }

    Engine::ECS::Entity create_object(s32 id)
    {
      if (id < 0 || id >= static_cast<s32>(object_list.size())) {
        Log::error("Invalid object ID: " + std::to_string(id));
        return Engine::ECS::Entity(); // Return invalid entity
      }

      ObjectPrefab& objectPrefab = object_list[id];

      // Create entity with Transform2D component using scale instead of size
      Engine::ECS::Entity entity = Engine::ECS::create_entity()
        .add_component<Components::ObjectId>(objectPrefab.object_id)
        .add_component<Components::Transform2D>(Math::Vec2D(0.0f, 0.0f), 0.0f, objectPrefab.size)
        .add_component<Components::Color>();

      if (objectPrefab.texture.has_value()) {
        entity.add_component<Components::Texture>(objectPrefab.texture.value());
      }

      if (objectPrefab.texture_atlas.has_value()) {
        entity.add_component<Components::TextureAtlas>(objectPrefab.texture_atlas.value());
      }

      if (objectPrefab.animation.has_value()) {
        entity.add_component<Components::Animation>(objectPrefab.animation.value());
      }

      return entity;
    }
  }
}