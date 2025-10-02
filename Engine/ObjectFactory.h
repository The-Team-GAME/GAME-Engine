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


namespace Engine
{

  struct ObjectPrefab
  {
    Components::ObjectId objectId; // so that level data can reference to these objects
    Components::Size size;
    std::optional<Components::StaticSprite> staticSprite;
    std::optional<Components::AnimatedSprite> animatedSprite;
  };

  class ObjectFactory
  {
  private:
    ECS::World* m_world;
    std::vector<ObjectPrefab> objectList;
    s32 nextObjectId = 0;
  public:

    ObjectFactory(ECS::World* world);


    s32 registerObject(std::string filePath);


    void unregisterObject(int id);


    void unregisterAllObjects();


    ECS::Entity createObject(int id);
  };
}