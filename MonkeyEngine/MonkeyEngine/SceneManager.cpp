//-----------------------------------------------------------------------------------
// file:    SceneManager.cpp
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (95%)
//          Primary author
//
//          Ryan Ang An Kiat (5%)
//          Secondary author
//          Added camera shake on enemy hit
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
#include "LevelManager.h"
#include "LevelTabManager.h"
#include "Logger.h"
#include "Systems/GraphicsSystem.h"
#include "EventManager/EventManager.h"
#include "Events/EngineEvents.h"

namespace Engine
{
  namespace Systems {
    void add_camera_shake(f32 amplitude, f32 duration) {
      CameraSystem::add_shake(amplitude, duration);
    }
  }

  namespace SceneManager
  {
    // Variable definitions - REQUIRED for extern variables
    Scene* current_scene = nullptr;
    std::unordered_map<std::string, std::shared_ptr<Scene>> scenes;
    std::vector<std::string> scene_order; // name of all scenes in order of registration
    std::vector<std::shared_ptr<Scene>> scene_stack;

    void load_scene(const std::string& name) {
      auto scene_it = scenes.find(name);
      if (scene_it == scenes.end()) {
        Log::error("Error: Scene '{}' not found!", name);
        return;
      }

      // Unload all scenes in stack
      while (!scene_stack.empty()) {
        scene_stack.back()->free();
        scene_stack.back()->unload();
        scene_stack.pop_back();
      }
      current_scene = nullptr;

      // Clear all level state before loading new scene
      // This prevents flickering between old and new levels
#ifndef GAME_BUILD
      Engine::LevelTabManager::clear_all();
#else
      Engine::LevelManager::clear_all();
#endif

      Engine::Systems::CameraSystem::reset_camera();

      // Push new scene
      auto new_scene = scene_it->second;
      scene_stack.push_back(new_scene);
      current_scene = new_scene.get();

      current_scene->load();
      current_scene->init();

      Log::info("Loaded scene: {}", name);
    }

    void push_scene(const std::string& name) {
      auto scene_it = scenes.find(name);
      if (scene_it == scenes.end()) {
        Log::error("Error: Scene '{}' not found!", name);
        Engine::EventManager::send_event<Engine::Events::ToastEvent>("Error: Scene '" + name + "' not found!", Engine::Events::ToastType::Error);
        return;
      }

      auto new_scene = scene_it->second;
      scene_stack.push_back(new_scene);
      current_scene = new_scene.get();

      current_scene->load();
      current_scene->init();

      Log::info("Pushed scene: {}", name);
    }

    void pop_scene() {
      if (scene_stack.empty()) {
        Log::error("pop_scene called but stack is empty!");
        return;
      }

      Log::info("Popping scene. Stack size before: {}", scene_stack.size());

      // Unload current
      scene_stack.back()->free();
      scene_stack.back()->unload();
      scene_stack.pop_back();

      if (!scene_stack.empty()) {
        current_scene = scene_stack.back().get();
        Log::info("Popped scene. Current: {}", get_current_scene_name());
      }
      else {
        current_scene = nullptr;
        Log::info("Popped last scene. Stack is empty.");
      }

      Log::info("Stack size after: {}", scene_stack.size());
    }

    void unload_current_scene() {
      if (current_scene != nullptr) {
        current_scene->free();
        current_scene->unload();
        // Remove from stack if it's the top one
        if (!scene_stack.empty() && scene_stack.back().get() == current_scene) {
          scene_stack.pop_back();
        }

        if (!scene_stack.empty()) {
          current_scene = scene_stack.back().get();
        }
        else {
          current_scene = nullptr;
        }

        Log::info("Unloaded current scene");
      }
    }

    void update() {
      if (current_scene != nullptr) {
        // Log::info("Updating scene: {}", get_current_scene_name()); // Spammy but useful for debugging
        current_scene->update();
      }
      else {
        Log::warning("SceneManager::update called but current_scene is null!");
      }
    }

    void draw() {
      for (auto& scene : scene_stack) {
        scene->draw();
      }
    }

    void cleanup() {
      while (!scene_stack.empty()) {
        scene_stack.back()->free();
        scene_stack.back()->unload();
        scene_stack.pop_back();
      }
      current_scene = nullptr;
      scenes.clear();
      scene_order.clear();
      Log::info("SceneManager cleaned up");
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