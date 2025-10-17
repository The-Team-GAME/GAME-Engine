//-----------------------------------------------------------------------------------
// file:    Prefab.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the Prefab system for the ECS. Prefabs are reusable
//          entity templates that store component configurations. They can be
//          instantiated multiple times to create entities with the same setup.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "Entity.h"
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <rapidjson/document.h>

namespace Engine {
  namespace ECS {

    /* ====================================== //
    // Component serialization registry       //
    // ====================================== */
    class ComponentSerializer {
    public:
      using SerializeFunc = std::function<void(EntityID, rapidjson::Value&, rapidjson::Document::AllocatorType&)>;

      static void register_serializer(ComponentTypeID type_id, const std::string& type_name, SerializeFunc func);

      template<typename T>
      static void register_component();

      static void serialize_component(ComponentTypeID type_id, EntityID entity, rapidjson::Value& json_val, rapidjson::Document::AllocatorType& allocator);
      static const std::string& get_type_name(ComponentTypeID type_id);

    private:
      static std::unordered_map<ComponentTypeID, SerializeFunc> serializers;
      static std::unordered_map<ComponentTypeID, std::string> type_names;
    };

    /* ====================================== //
    // Prefab - Reusable entity template      //
    // ====================================== */
    struct Prefab {
      std::function<void(Entity&)> setup_function;

      Prefab() = default;
      Prefab(std::function<void(Entity&)> setup);
    };

    /* ====================================== //
    // Prefab functions                       //
    // ====================================== */
    Prefab create_prefab(std::function<void(Entity&)> setup);
    Entity instantiate_prefab(const Prefab& prefab);
    std::string serialize_prefab_to_json(const Prefab& prefab);

  } // namespace ECS
} // namespace Engine