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
#include "EngineExport.h"


namespace Engine
{
  /**
   * @brief Abstract base class for all game scenes.
   * @details Defines the required interface for scene lifecycle management.
   */
  class ENGINE_API Scene {
  public:
    virtual ~Scene() = default;

    // If true, SceneManager will not draw any scenes below this one in the stack.
    // Useful for full-screen opaque overlays like cutscenes.
    virtual bool blocks_lower_draw() const { return false; }

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
    ENGINE_API extern Scene* current_scene;
    ENGINE_API extern std::unordered_map<std::string, std::shared_ptr<Scene>> scenes;
    ENGINE_API extern std::vector<std::string> scene_order;
    ENGINE_API extern std::vector<std::shared_ptr<Scene>> scene_stack;

    /**
     * @brief Registers a new scene type with the manager.
     * @tparam T Scene type to register (must derive from Scene).
     * @param name Name of the scene.
     */
    template<typename T>
    void register_scene(const std::string& name) {
      static_assert(std::is_base_of<Scene, T>::value,
        "T must derive from Scene");
      scenes[name] = std::make_shared<T>();
      scene_order.push_back(name);
    }

    /**
     * @brief Loads the scene with the given name and sets it as current.
     * @param name Name of the scene to load.
     */
    ENGINE_API void load_scene(const std::string& name);

    /**
     * @brief Pushes a scene onto the stack without unloading the current one.
     * @param name Name of the scene to push.
     */
    ENGINE_API void push_scene(const std::string& name);

    /**
     * @brief Pops the top scene from the stack and resumes the previous one.
     */
    ENGINE_API void pop_scene();

    /**
     * @brief Unloads the current scene and performs cleanup.
     */
    ENGINE_API void unload_current_scene();

    /**
     * @brief Updates the current scene each frame.
     */
    ENGINE_API void update();

    /**
     * @brief Draws the current scene (to be removed soon).
     */
    ENGINE_API void draw();

    /**
     * @brief Cleans up all scenes and releases resources.
     */
    ENGINE_API void cleanup();

    /**
     * @brief Gets the name of the current scene.
     * @return Name of the current scene.
     */
    ENGINE_API std::string get_current_scene_name();

    /**
     * @brief Gets a list of all registered scene names.
     * @return Vector of scene names.
     */
    ENGINE_API std::vector<std::string> get_all_scene_names();
  }
}