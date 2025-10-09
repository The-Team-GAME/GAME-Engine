#pragma once

#include <vector>
#include "../ECS/ECS.h"

// NOT USED YET, WILL BE IMPLEMENTED IN THE FUTURE
namespace Components
{
  struct Parent {
    Engine::ECS::Entity parent_id;
  };
  struct Children { std::vector<Engine::ECS::Entity> child_ids; };
  struct Root {}; // Mark root entities
  struct Sibling { Engine::ECS::Entity next_sibling; }; // Maybe don't need this
}