#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <type_traits>


namespace Engine
{
  class Scene {
  public:
    virtual ~Scene() = default;

    virtual void load() = 0;
    virtual void init() = 0;

    virtual void update() = 0;

    //removing draw soon
    virtual void draw() = 0;

    virtual void free() = 0;

    virtual void unload() = 0;


  };


  namespace SceneManager {
    // Declare as extern - actual definition in SceneManager.cpp
    extern Scene* current_scene;
    extern std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
    extern std::vector<std::string> scene_order;

    // Template function must be defined in header
    template<typename T>
    void register_scene(const std::string& name) {
      static_assert(std::is_base_of<Scene, T>::value,
        "T must derive from Scene");
      scenes[name] = std::make_unique<T>();
      scene_order.push_back(name);
    }

    // Function declarations
    void load_scene(const std::string& name);
    void unload_current_scene();
    void update();
    void draw();
    void cleanup();
    std::string get_current_scene_name();
    std::vector<std::string> get_all_scene_names();
  }
}