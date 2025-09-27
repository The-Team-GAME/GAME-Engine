#pragma once
#include "Types.h"

namespace ECS {

  // Automatic component type ID generation
  class ComponentTypeManager {
  private:
    static ComponentTypeID nextTypeId;

  public:
    template<typename T>
    static ComponentTypeID getTypeId();

    static ComponentTypeID getNextAvailableId() {
      return nextTypeId + 1;
    }
  };

} // namespace ECS

