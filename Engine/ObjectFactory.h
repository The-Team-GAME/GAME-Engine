//-----------------------------------------------------------------------------------
// file:    ObjectFactory.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the ObjectFactory namespace which manages object prefabs
//          and creates ECS entities from JSON configuration files. It handles loading
//          textures, sprite sheets, and component initialization.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <vector>
#include "ECS/ECS.h"
#include "Components.h"
#include <optional>
#include "TypeDef.h"

namespace Engine
{
  struct ObjectPrefab
  {
    Components::ObjectId object_id;
    Math::Vec2D size;
    std::optional<Engine::Graphics::Texture> texture;
    std::optional<Components::TextureAtlas> texture_atlas;
    std::optional<Components::Animation> animation;
  };

  namespace ObjectFactory
  {
    /*=================================================================================
    Function Brief:
    Registers an object prefab from a JSON file. The JSON file should contain entity
    configuration data including size, texture paths, and sprite sheet information.

    Parameters:
    filePath - The path to the JSON file describing the object's components.

    Returns:
    An integer ID for the newly registered object prefab.

    Where to call it:
    During game initialization or when loading new object types. Call this before
    attempting to create instances of the object with create_object().
    ==================================================================================*/
    s32 register_object(const std::string& filePath);

    /*=================================================================================
    Function Brief:
    Unregisters an object prefab using its ID, removing it from the factory's
    internal list.

    Parameters:
    id - The ID of the object prefab to unregister.

    Where to call it:
    When an object type is no longer needed or during cleanup operations.
    ==================================================================================*/
    void unregister_object(s32 id);

    /*=================================================================================
    Function Brief:
    Unregisters all object prefabs, clearing the factory's internal list and
    resetting the ID counter.

    Where to call it:
    During level transitions, game shutdown, or when a complete reset of registered
    objects is required.
    ==================================================================================*/
    void unregister_all_objects();

    /*=================================================================================
    Function Brief:
    Creates a new entity in the ECS world based on a registered object prefab.
    The entity will be initialized with all components defined in the prefab.

    Parameters:
    id - The ID of the object prefab to instantiate.

    Returns:
    The newly created ECS entity with all components from the prefab attached.

    Where to call it:
    During gameplay when spawning new game objects, such as enemies, collectibles,
    or environmental elements.
    ==================================================================================*/
    ECS::Entity create_object(s32 id);
  }
}