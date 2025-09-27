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