#pragma once

#include <vector>
#include "ECS/ECS.h"
#include "BasicComponents.h"
#include <optional>

namespace
{
  struct ObjectPrefab
  {
    Components::ObjectId objectId; // so that level data can reference to these objects
    Components::Size size;
    std::optional<Components::StaticSprite> staticSprite;
  };
}

namespace Engine
{
  /**
   * @class ObjectFactory
   * @brief A factory for creating game objects (entities) from prefabs.
   *
   * This class is responsible for loading object prefabs from files,
   * storing them, and then creating entities in the ECS world based on these prefabs.
   */
  class ObjectFactory
  {
  private:
    ECS::World* m_world;
    std::vector<ObjectPrefab> objectList;
    int nextObjectId = 0;
  public:
    /**
     * @brief Construct a new Object Factory object.
     *
     * @param world A pointer to the ECS world where entities will be created.
     */
    ObjectFactory(ECS::World* world);

    /**
     * @brief Registers an object prefab from a JSON file. An object prefab is akin to the entity type.
     *
     * @param filePath The path to the JSON file describing the object's components.
     * @return int An ID for the newly registered object prefab.
     */
    int registerObject(std::string filePath);

    /**
     * @brief Unregisters an object prefab using its ID.
     *
     * @param id The ID of the object prefab to unregister.
     */
    void unregisterObject(int id);

    /**
     * @brief Creates a unique entity in the world from a registered object prefab.
     *
     * @param id The ID of the object prefab to instantiate.
     * @return ECS::Entity The newly created entity with components from the prefab.
     */
    ECS::Entity createObject(int id);
  };
}