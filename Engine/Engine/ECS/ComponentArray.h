//-----------------------------------------------------------------------------------
// file:    ComponentArray.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the component storage interface and type-safe component
//          array template class for the ECS (Entity Component System). ComponentArray
//          stores components of a specific type and provides fast lookups by entity ID.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include <unordered_map>
#include <memory>

namespace Engine {
  namespace ECS {

    /* ============================ //
    // Component storage interface  //
    // ============================ */
    class IComponentArray {
    public:
      virtual ~IComponentArray() = default;
      virtual void remove_entity(EntityID entity) = 0;
      virtual size_t size() const = 0;
      virtual void clone_component(EntityID source, EntityID destination) = 0;
    };

    /* ============================= //
    // Type-safe component storage   //
    // ============================= */
    template<typename T>
    class ComponentArray : public IComponentArray {
    private:
      std::unordered_map<EntityID, T> components;

    public:
      void add_component(EntityID entity, T component);

      void remove_component(EntityID entity);

      void clone_component(EntityID source, EntityID destination) override;

      T* get_component(EntityID entity);

      bool has_component(EntityID entity) const;

      void remove_entity(EntityID entity) override;

      size_t size() const override;

      auto begin();
      auto end();
      auto begin() const;
      auto end() const;
    };

  } // namespace ECS
} // namespace Engine