//-----------------------------------------------------------------------------------
// file:    World.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the World namespace which is the main ECS container.
//          Manages entities, components, systems, and events. Provides the primary
//          interface for creating entities, adding components, registering systems,
//          and dispatching events.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "ComponentManager.h"
#include "ComponentArray.h"
#include "Entity.h"
#include "System.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <cassert>
#include <functional>

namespace Engine {
  namespace ECS
  {
    /* ========================= //
    // Component registration    //
    // ========================= */
    template <typename T>
    void register_component();

    template <typename T>
    ComponentArray<T>* get_component_array();

    /* =================== //
    // Entity management   //
    // =================== */
    Entity create_entity();
    Entity get_entity(EntityID id);
    Entity clone_entity(const Entity& source);
    void destroy_entity(const Entity& entity);

    template <typename... T>
    void destroy_entities_with();

    bool is_entity_active(const Entity& entity);

    /* ====================== //
    // Component management   //
    // ====================== */
    template <typename T, typename... Args>
    void add_component(const Entity& entity, Args &&...args);

    template <typename T>
    void remove_component(const Entity& entity);

    template <typename T>
    T* get_component(const Entity& entity);

    template <typename... T>
    bool has_components(const Entity& entity);

    template <typename T>
    bool has_component(const Entity& entity);

    /* ================== //
    // System management  //
    // ================== */
    template <typename T, typename... Args>
    T* add_system(Args &&...args);

    template <typename T>
    void remove_system();

    /* ==================== //
    // Main update loop     //
    // ==================== */
    void update();

    /* ================ //
    // Main draw loop   //
    // ================ */
    void draw();

    /* ============================================== //
    // Query methods - return Entity instead of EntityID //
    // ============================================== */
    template <typename... T>
    std::vector<Entity> get_entities_with();

    template <typename... T>
    Entity get_first_entity_with();

    std::vector<Entity> get_all_entities();
    size_t get_entity_count();

    template <typename T>
    size_t get_component_count();

    /* =============== //
    // Internal state  //
    // =============== */
    namespace Internal
    {
      extern EntityID next_entity_id;
      extern std::unordered_set<EntityID> active_entities;
      extern std::unordered_set<EntityID> entities_to_destroy;
      extern std::unordered_map<EntityID, ComponentSignature> entity_signatures;
      extern std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>> component_arrays;
      extern std::vector<std::unique_ptr<System>> systems;

      void entity_component_changed(EntityID entity);
      bool system_matches_entity(const System& system, EntityID entity);
    }
  } // namespace ECS
} // namespace Engine

