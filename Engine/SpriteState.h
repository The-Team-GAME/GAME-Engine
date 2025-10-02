//-----------------------------------------------------------------------------------
// file:    SpriteState.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file defines the SpriteState class, which manages the main scene,
//          loads visual assets, handles file browsing, processes drag-and-drop
//          events from the SpriteDropper, and orchestrates the Timeline and object
//          creation logic within the game loop.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------


#pragma once

#include "StateManager.h"
#include "core.h"
#include "ObjectFactory.h"
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <stack>
#include <iostream>
#include "FileExplorer.h"
#include "SpriteDropper.h"
#include "Timeline.h"
#include "Time.h"



namespace fs = std::filesystem;

// images selection pathway(currently unused)
//extern std::string selected_file_path;

// The directory where the project is located, selected by the user
extern std::string root_path;






class SpriteState : public Engine::GameState<SpriteState> {

private:

  Timeline timeline;	// adds timeline class to file

  int linearMonkeyImageId = -1;
  int nearestMonkeyImageId = -1;
  std::vector<int> loadedImageIds;
  std::vector<DroppedImage> droppedImages;

  bool showFileExplorerSource = false;
  bool fileCopied = false;

  struct ImageData {
    unsigned int texture_id;
    const char* name;
  };

  std::vector<ImageData> ImageAspectRatio;
  float spritePosX = 0.0f;


  /*=================================================================================
    Function Brief:
    Clears the current list of loaded texture IDs and reloads all image files from
    the project's asset directory into the graphics system.

    Where to call it:
    Called on game load, after a new image file is successfully copied into the
    assets folder, or whenever the asset list needs to be refreshed.
    ==================================================================================*/
  void LoadAllAssets() {
    loadedImageIds.clear();
    fs::path assets_path = fs::current_path() / "Engine" / "Assets" / "images";
    if (!fs::exists(assets_path) || !fs::is_directory(assets_path)) {
      fs::create_directories(assets_path);
      return;
    }
    for (const auto& entry : fs::directory_iterator(assets_path)) {
      if (entry.is_regular_file()) {
        std::string extension = entry.path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (extension == ".jpg" || extension == ".png" || extension == ".jpeg") {
          int textureId = Engine::GRAPHICS.load_texture(entry.path().string().c_str(), Graphics::MinifyingFilter::Nearest);
          if (textureId != -1) {
            loadedImageIds.push_back(textureId);
          }
        }
      }
    }
  }




public:

  /*=================================================================================
    Function Brief:
    Called when the game state is loaded. This is typically used for background
    loading tasks. Here, it loads all available project assets.

    Where to call it:
    Automatically called by the StateManager when transitioning to this state.
    ==================================================================================*/
  void Load() {
    LoadAllAssets(); //loads updating assets within sprite dropper
  };


  /*=================================================================================
    Function Brief:
    Initializes the game state by loading necessary starting assets and setting up
    initial variables.

    Where to call it:
    Automatically called by the StateManager right after Load() and before Update().
    ==================================================================================*/
  void Init() {
    linearMonkeyImageId = Engine::GRAPHICS.load_texture("Assets/images/monkey.png");
    nearestMonkeyImageId = Engine::GRAPHICS.load_texture("Assets/images/monkey.png", Graphics::MinifyingFilter::Nearest);
    std::cout << "Current path is " << std::filesystem::current_path() << std::endl;
  };


  /*=================================================================================
    Function Brief:
    The main game loop logic update function. Handles non-rendering updates like
    physics and input processing.

    Where to call it:
    Called once per game loop frame by the StateManager.
    ==================================================================================*/
  void Update() {



  };


  /*=================================================================================
    Function Brief:
    The main rendering function. Clears the screen, draws all game objects, and
    handles ImGui interface rendering.

    Where to call it:
    Called once per game loop frame by the StateManager after Update().
    ==================================================================================*/
  void Draw() {
    // Main scene texture drawing
    Engine::GRAPHICS.clear_screen(0.1f, 0.1f, 0.1f, 1.0f);
    Engine::GRAPHICS.draw_texture(linearMonkeyImageId, 200.0f, 150.0f, 150.0f, 210.0f);
    Engine::GRAPHICS.draw_texture(nearestMonkeyImageId, 400.0f, 150.0f, 150.0f, 210.0f);
    Engine::GRAPHICS.draw_texture_region(
      nearestMonkeyImageId, 600.0f, 150.0f, 150.0f, 210.0f, spritePosX += Engine::Time::get_fixed_dt() * 10.0f, 0.0f, 15.0f, 21.0f
    );



    // This loop draws the dropped images at their final position
    for (const auto& droppedImage : droppedImages) {
      //Engine::GRAPHICS.draw_texture(droppedImage.textureId, droppedImage.position.x, droppedImage.position.y, 150.0f, 210.0f);
      Engine::GRAPHICS.draw_texture(droppedImage.textureId, 600.f, 500.f, 150.0f, 210.0f);
    }

    timeline.Draw();

    // --- Image Menu ---
    ImGui::Begin("Image Menu");
    ImGui::Text("Project Root: %s", fs::current_path().string().c_str());
    ImGui::Separator();
    if (ImGui::Button("Browse for Image")) {
      showFileExplorerSource = true;
      fileCopied = false;
      selected_file_path.clear();
    }

    if (showFileExplorerSource) {
      FileExplorer(&showFileExplorerSource);
    }

    if (!showFileExplorerSource && !selected_file_path.empty() && !fileCopied) {
      try {
        fs::path source_path(selected_file_path);
        fs::path destination_dir = fs::current_path() / "Engine" / "Assets" / "images";
        fs::path destination_path = destination_dir / source_path.filename();
        fs::create_directories(destination_dir);
        fs::copy_file(source_path, destination_path, fs::copy_options::overwrite_existing);
        std::cout << "File copied successfully to: " << destination_path << std::endl;
        LoadAllAssets();
        selected_file_path.clear();
        fileCopied = true;
      }
      catch (const fs::filesystem_error& e) {
        fprintf(stderr, "Filesystem error: %s\n", e.what());
      }
    }
    ImGui::End();

    //calls the sprite dropper function :>
    SpriteDropper(loadedImageIds, droppedImages);

  };

  /*=================================================================================
    Function Brief:
    Called when the game state is paused or when Unload() is about to be called.
    Use this to free up temporary resources or save transient state data.

    Where to call it:
    Automatically called by the StateManager.
    ==================================================================================*/
  void Free() {};

  /*=================================================================================
    Function Brief:
    Called when the game state is permanently exiting. Use this to unload or
    destroy all resources (textures, models, etc.) specific to this state.

    Where to call it:
    Automatically called by the StateManager when leaving this state.
    ==================================================================================*/
  void Unload() {};
};