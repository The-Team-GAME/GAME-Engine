//-----------------------------------------------------------------------------------
// file:    ObjectFactory.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the ObjectFactory class which manages object prefabs
//          and creates ECS entities from JSON configuration files. It handles loading
//          textures, sprite sheets, and component initialization.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <vector>
#include "ECS/ECS.h"
#include "BasicComponents.h"
#include <optional>
#include "TypeDef.h"


namespace Engine
{

  struct ObjectPrefab
  {
    Components::ObjectId object_id; // so that level data can reference to these objects
    Components::Size size;
    std::optional<Components::StaticSprite> static_sprite;
    std::optional<Components::AnimatedSprite> animated_sprite;
  };

  class ObjectFactory
  {
  private:
    ECS::World* m_world;
    std::vector<ObjectPrefab> objectList;
    s32 nextObjectId = 0;
  public:
    /*=================================================================================
    Function Brief:
    Constructs a new ObjectFactory instance that will create entities in the
    specified ECS world.

    Parameters:
    world - A pointer to the ECS world where entities will be created.
    ==================================================================================*/
    ObjectFactory(ECS::World* world);

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
    attempting to create instances of the object with createObject().
    ==================================================================================*/
    s32 registerObject(std::string filePath);

    /*=================================================================================
    Function Brief:
    Unregisters an object prefab using its ID, removing it from the factory's
    internal list.

    Parameters:
    id - The ID of the object prefab to unregister.

    Where to call it:
    When an object type is no longer needed or during cleanup operations.
    ==================================================================================*/
    void unregisterObject(s32 id);

    /*=================================================================================
    Function Brief:
    Unregisters all object prefabs, clearing the factory's internal list and
    resetting the ID counter.

    Where to call it:
    During level transitions, game shutdown, or when a complete reset of registered
    objects is required.
    ==================================================================================*/
    void unregisterAllObjects();

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
    ECS::Entity createObject(s32 id);
  };
}