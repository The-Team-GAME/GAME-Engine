#pragma once

#include <string>
#include <vector>
#include "Reflection/ReflectionLibrary/GenericValue.h"
#include "ECS/ECS.h"


namespace Engine {

  struct Level {
    std::string name;
    std::vector<ReflectionLibrary::GenericValue> entities;

    static Level from_world();

    void instantiate_into_world();

  };

}