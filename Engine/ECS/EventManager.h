#pragma once

#include "Types.h"
#include <any>
#include <functional>
#include <unordered_map>
#include <vector>

namespace ECS
{

  // Automatic event type ID generation
  class EventTypeManager
  {
  private:
    static EventTypeID nextTypeId;

  public:
    template <typename T>
    static EventTypeID getTypeId()
    {
      static EventTypeID typeId = ++nextTypeId;
      return typeId;
    }
  };

  // A wrapper for a specific event instance
  struct Event
  {
    EventTypeID typeId = INVALID_EVENT_TYPE;
    std::any data;

    template <typename T>
    Event(T&& eventData) : typeId(EventTypeManager::getTypeId<T>()), data(std::forward<T>(eventData)) {}

    template <typename T>
    const T* get() const
    {
      if (EventTypeManager::getTypeId<T>() == typeId)
      {
        return std::any_cast<T>(&data);
      }
      return nullptr;
    }
  };

} // namespace ECS