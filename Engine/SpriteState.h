//-----------------------------------------------------------------------------------
// file:    SpriteState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a game state for demonstrating sprite rendering with
//          different texture filtering modes and includes a file explorer utility
//          for browsing and selecting image files.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
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
#include <iostream> // For debugging output



namespace fs = std::filesystem;



// The file path of the image that was selected

static std::string selected_file_path;

// The directory where the project is located, selected by the user

static std::string root_path;



/*=================================================================================
Function Brief:
Creates an ImGui-based file explorer dialog for browsing and selecting files or
directories. Supports filtering for image files and directory navigation with
history tracking.

Parameters:
p_open - Pointer to boolean controlling whether the dialog is open
is_directory_selection_mode - If true, allows selecting folders; if false, filters
                              for image files only

Where to call it:
Call this function each frame when the file explorer should be visible, typically
from within an ImGui drawing context.
==================================================================================*/
void file_explorer(bool* p_open, bool is_directory_selection_mode = false) {

  static fs::path current_path;
  static bool first_run = true;
  static std::stack<fs::path> history;


  if (first_run) {
    first_run = false;
    fs::path home_path;

#ifdef _WIN32

    char* user_profile = nullptr;
    size_t size = 0;

    if (_dupenv_s(&user_profile, &size, "USERPROFILE") == 0 && user_profile != nullptr) {
      home_path = fs::path(user_profile) / fs::current_path();
      free(user_profile);
    }

#else
    const char* home_dir = getenv("HOME");
    if (home_dir) {
      home_path = fs::path(home_dir) / "Downloads";
    }

#endif

    if (home_path.empty()) {
      current_path = fs::current_path();
    }

    else {

      current_path = home_path;

    }

  }



  if (ImGui::Begin("File Explorer", p_open)) {

    if (!history.empty() && ImGui::Button("Back")) {

      current_path = history.top();

      history.pop();

    }

    ImGui::SameLine();


    //goes to desktop
    if (ImGui::Button("..")) {

      if (current_path.has_parent_path()) {

        history.push(current_path);

        current_path = current_path.parent_path();

      }

    }



    ImGui::Separator();



    if (is_directory_selection_mode) {

      ImGui::Text("Select a folder:");

      //selects current folder
      if (ImGui::Button("Select This Folder")) {

        root_path = current_path.string();

        *p_open = false;

      }

    }

    else {

      ImGui::Text("Select an image file:");

    }



    ImGui::Text("Current Path: %s", current_path.string().c_str());



    if (!fs::exists(current_path) || !fs::is_directory(current_path)) {

      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Invalid directory path!");

      ImGui::End();

      return;

    }


    // checks pathways if files are .jpg .png .jpeg
    std::vector<fs::directory_entry> entries;
    for (const auto& entry : fs::directory_iterator(current_path)) {

      if (fs::is_directory(entry.path())) {

        entries.push_back(entry);

      }

      else if (!is_directory_selection_mode) {

        std::string extension = entry.path().extension().string();

        std::transform(extension.begin(), extension.end(), extension.begin(),
          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (extension == ".jpg" || extension == ".png" || extension == ".jpeg") {

          entries.push_back(entry);

        }

      }

    }


    //sorts pathways
    std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {

      bool a_is_dir = fs::is_directory(a.path());

      bool b_is_dir = fs::is_directory(b.path());

      if (a_is_dir && !b_is_dir) { return true; }

      if (!a_is_dir && b_is_dir) { return false; }

      return a.path().filename().string() < b.path().filename().string();

      });



    for (const auto& entry : entries) {

      bool is_directory = fs::is_directory(entry.path());

      const char* icon = is_directory ? "[DIR]" : "[FILE]";



      if (ImGui::Selectable((icon + std::string(" ") + entry.path().filename().string()).c_str(), false, 0, ImVec2(0, 20))) {

        if (is_directory) {

          history.push(current_path);

          current_path /= entry.path().filename();

        }

        else {

          if (!is_directory_selection_mode) {

            selected_file_path = entry.path().string();

            *p_open = false;

          }

        }

      }

    }

  }

  ImGui::End();

}

class SpriteState : public Engine::GameState<SpriteState> {
private:
  s32 linear_monkey_image_id = -1;
  s32 nearest_monkey_image_id = -1;
  bool show_file_explorer_source = false;
  bool show_file_explorer_dest = false;
  bool file_copied = false;

  struct ImageData {
    u32 texture_id;
    const char* name;
  };

  std::vector<ImageData> image_aspect_ratio;
  f32 sprite_pos_x = 0.0f;

public:
  void Load() {};

  void Init() {
    // Load textures using ResourceManager
    linear_monkey_image_id = Engine::RESOURCES.load_texture("monkey_linear", "Assets/images/monkey.png", Graphics::MinifyingFilter::Linear);

    nearest_monkey_image_id = Engine::RESOURCES.load_texture("monkey_nearest", "Assets/images/monkey.png", Graphics::MinifyingFilter::Nearest);

    std::cout << "Current path is " << std::filesystem::current_path() << std::endl;
  };

  void Update() {};

  void Draw() {
    Engine::GRAPHICS.clear_screen(0.1f, 0.1f, 0.1f, 1.0f);

    Engine::GRAPHICS.draw_texture(linear_monkey_image_id, 200.0f, 150.0f, 150.0f, 210.0f);
    Engine::GRAPHICS.draw_texture(nearest_monkey_image_id, 400.0f, 150.0f, 150.0f, 210.0f);

    Engine::GRAPHICS.draw_texture_region(
      nearest_monkey_image_id, 600.0f, 150.0f, 150.0f, 210.0f, sprite_pos_x += Engine::Time::get_fixed_dt() * 10.0f, 0.0f, 15.0f, 21.0f
    );



    //image menu

    ImGui::Begin("Image Menu");

    //root

    ImGui::Text("Project Root: %s", root_path.empty() ? "Not Set" : root_path.c_str());

    if (ImGui::Button("Set Project Root")) {

      show_file_explorer_dest = true;
      file_copied = false;
    }



    ImGui::Separator();



    //image browser

    ImGui::Begin("Image Menu");



    // The button that triggers the popup

    if (ImGui::Button("Browse for Image")) {

      if (root_path.empty()) {

        // Open the popup when this button is clicked and the project path isn't set.

        ImGui::OpenPopup("No Root File Selected");

      }

      else {

        show_file_explorer_source = true;

        file_copied = false;

        selected_file_path.clear();

      }

    }

    ImGui::End();



    // The popup window definition must be placed here, outside the button's logic.

    // ImGui::BeginPopupModal checks if the popup is open and renders its contents.

    if (ImGui::BeginPopupModal("No Root File Selected", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

      ImGui::Text("Please set the project root directory first.");

      ImGui::Separator();



      // This button closes the current popup.

      if (ImGui::Button("Okay", ImVec2(120, 0))) {

        ImGui::CloseCurrentPopup();

      }

      ImGui::EndPopup();

    }



    if (show_file_explorer_source) {

      file_explorer(&show_file_explorer_source);

    }
    if (show_file_explorer_dest) {

      file_explorer(&show_file_explorer_dest, true);

    }



    // Logic to copy the file after both paths are set

    if (!show_file_explorer_source && !show_file_explorer_dest && !selected_file_path.empty() && !root_path.empty() && !file_copied) {

      try {

        //setting root to the pathway of github to add any images to the�

        fs::path source_path(selected_file_path);

        fs::path destination_dir = fs::path(root_path) / "Engine" / "Assets" / "images";

        fs::path destination_path = destination_dir / source_path.filename();



        fs::create_directories(destination_dir);

        fs::copy(source_path, destination_path, fs::copy_options::overwrite_existing);



        std::cout << "File copied successfully to: " << destination_path << std::endl;



        selected_file_path.clear();

        file_copied = true;

      }

      catch (const fs::filesystem_error& e) {

        fprintf(stderr, "Filesystem error: %s\n", e.what());

      }

    }

    ImGui::End();



    //sprite dropper

    ImGui::Begin("Sprite Dropper");

    ImGui::Text("Current State: %s", typeid(*Engine::StateManager::GetCurrentState()).name());

    if (ImGui::Button("Cool")) { Engine::StateManager::ChangeState(BasicState::Get()); }

    if (ImGui::Button("Huh")) { Engine::StateManager::ChangeState(BasicState::Get()); }

    if (ImGui::Button("Fweh")) { Engine::StateManager::ChangeState(BasicState::Get()); }

    ImGui::End();

  };



  void Free() {};

  void Unload() {
    // Unload textures using ResourceManager
    Engine::RESOURCES.unload_texture("monkey_linear");
    Engine::RESOURCES.unload_texture("monkey_nearest");
  };
};