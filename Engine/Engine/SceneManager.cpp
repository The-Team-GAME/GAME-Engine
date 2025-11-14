//-----------------------------------------------------------------------------------
// file:    SceneManager.cpp
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   Implements the SceneManager for the game engine. Handles scene registration,
//          loading, unloading, updating, drawing, and cleanup. Manages the current scene
//          pointer and provides access to scene names and order.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "SceneManager.h"
#include <iostream>
#include "Systems/GraphicsSystem.h"

namespace Engine
{
  namespace SceneManager
  {
    // Variable definitions - REQUIRED for extern variables
    Scene* current_scene = nullptr;
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
    std::vector<std::string> scene_order; // name of all scenes in order of registration

    void load_scene(const std::string& name) {
      auto scene_it = scenes.find(name);
      if (scene_it == scenes.end()) {
        std::cerr << "Error: Scene '" << name << "' not found!" << std::endl;
        return;
      }

      unload_current_scene();

      Engine::Systems::CameraSystem::reset_camera();

      current_scene = scene_it->second.get();
      current_scene->load();
      current_scene->init();

      Engine::LevelManager::set_current_level_path(""); // TODO: very hacky, we need a better way to manage levels vs scenes

      std::cout << "Loaded scene: " << name << std::endl;
    }

    void unload_current_scene() {
      if (current_scene != nullptr) {
        current_scene->free();
        current_scene->unload();
        current_scene = nullptr;
        std::cout << "Unloaded current scene" << std::endl;
      }
    }

    void update() {
      if (current_scene != nullptr) {
        current_scene->update();
      }
    }

    void draw() {
      if (current_scene != nullptr) {
        current_scene->draw();
      }
    }

    void cleanup() {
      unload_current_scene();
      scenes.clear();
      scene_order.clear();
      std::cout << "SceneManager cleaned up" << std::endl;
    }

    std::string get_current_scene_name() {
      for (const auto& pair : scenes) {
        if (pair.second.get() == current_scene) {
          return pair.first;
        }
      }
      return "No current scene";
    }

    std::vector<std::string> get_all_scene_names() {
      return scene_order;
    }
  }
}