//-----------------------------------------------------------------------------------
// file:    SceneManager.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   Declares the Scene base class and SceneManager namespace for managing
//          game scenes. Handles scene registration, loading, updating, drawing,
//          cleanup, and scene order tracking.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <type_traits>


namespace Engine
{
  /**
   * @brief Abstract base class for all game scenes.
   * @details Defines the required interface for scene lifecycle management.
   */
  class Scene {
  public:
    virtual ~Scene() = default;

    /**
     * @brief Loads resources and entities for the scene.
     */
    virtual void load() = 0;

    /**
     * @brief Initializes the scene after loading.
     */
    virtual void init() = 0;

    /**
     * @brief Updates the scene logic each frame.
     */
    virtual void update() = 0;

    /**
     * @brief Draws the scene (to be removed soon).
     */
    virtual void draw() = 0;

    /**
     * @brief Frees resources and entities for the scene.
     */
    virtual void free() = 0;

    /**
     * @brief Unloads the scene and performs cleanup.
     */
    virtual void unload() = 0;
  };

  namespace SceneManager {
    // Declare as extern - actual definition in SceneManager.cpp
    extern Scene* current_scene;
    extern std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
    extern std::vector<std::string> scene_order;

    /**
     * @brief Registers a new scene type with the manager.
     * @tparam T Scene type to register (must derive from Scene).
     * @param name Name of the scene.
     */
    template<typename T>
    void register_scene(const std::string& name) {
      static_assert(std::is_base_of<Scene, T>::value,
        "T must derive from Scene");
      scenes[name] = std::make_unique<T>();
      scene_order.push_back(name);
    }

    /**
     * @brief Loads the scene with the given name and sets it as current.
     * @param name Name of the scene to load.
     */
    void load_scene(const std::string& name);

    /**
     * @brief Unloads the current scene and performs cleanup.
     */
    void unload_current_scene();

    /**
     * @brief Updates the current scene each frame.
     */
    void update();

    /**
     * @brief Draws the current scene (to be removed soon).
     */
    void draw();

    /**
     * @brief Cleans up all scenes and releases resources.
     */
    void cleanup();

    /**
     * @brief Gets the name of the current scene.
     * @return Name of the current scene.
     */
    std::string get_current_scene_name();

    /**
     * @brief Gets a list of all registered scene names.
     * @return Vector of scene names.
     */
    std::vector<std::string> get_all_scene_names();
  }
}