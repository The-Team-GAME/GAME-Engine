#include "SceneManager.h"
#include <iostream>

namespace Engine
{
  namespace SceneManager 
  {
    // Variable definitions - REQUIRED for extern variables
    Scene* current_scene = nullptr;
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;

    void load_scene(const std::string& name) {
      auto scene_it = scenes.find(name);
      if (scene_it == scenes.end()) {
        std::cerr << "Error: Scene '" << name << "' not found!" << std::endl;
        return;
      }

      unload_current_scene();

      current_scene = scene_it->second.get();
      current_scene->load();
      current_scene->init();
      
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
      std::vector<std::string> names;
      for (const auto& pair : scenes) {
        names.push_back(pair.first);
      }
      return names;
    }
  }
}