//-----------------------------------------------------------------------------------
// file:    EventManager.inl
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the template methods for EventTypeManager and Event.
//          Provides automatic event type ID generation and type-safe event data
//          storage and retrieval.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "EventManager.h"

namespace ECS
{

  template <typename T>
  EventTypeID EventTypeManager::getTypeId()
  {
    static EventTypeID typeId = ++nextTypeId;
    return typeId;
  }

  template <typename T>
  Event::Event(T&& eventData) : typeId(EventTypeManager::getTypeId<T>()), data(std::forward<T>(eventData)) {}

  template <typename T>
  const T* Event::get() const
  {
    if (EventTypeManager::getTypeId<T>() == typeId)
    {
      return std::any_cast<T>(&data);
    }
    return nullptr;
  }

} // namespace ECS