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
// This variable is now unused, as the asset path is hardcoded
static std::string root_path;



// My file explorer function :>
void FileExplorer(bool* p_open) {
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
      home_path = fs::path(user_profile) / "Downloads";
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
    ImGui::Text("Select an image file:");

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
      else {
        std::string extension = entry.path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
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
          selected_file_path = entry.path().string();
          *p_open = false;
        }
      }
    }
  }
  ImGui::End();
}




class SpriteState : public Engine::GameState<SpriteState> {

private:
  int linearMonkeyImageId = -1;
  int nearestMonkeyImageId = -1;
  std::vector<int> loadedImageIds; //vector id for all my new loaded images
  bool showFileExplorerSource = false;
  bool fileCopied = false;

  struct ImageData {
    unsigned int texture_id;
    const char* name;
  };


  std::vector<ImageData> ImageAspectRatio;
  float spritePosX = 0.0f;


  // Helper function to load all images from the assets directory
  void LoadAllAssets() {
    loadedImageIds.clear();
    fs::path assets_path = fs::current_path() / "Engine" / "Assets" / "images";
    if (!fs::exists(assets_path) || !fs::is_directory(assets_path)) {
      // Create the directory if it doesn't exist
      fs::create_directories(assets_path);
      return;
    }
    for (const auto& entry : fs::directory_iterator(assets_path)) {
      if (entry.is_regular_file()) {
        std::string extension = entry.path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        if (extension == ".jpg" || extension == ".png" || extension == ".jpeg") {
          int textureId = Engine::GRAPHICS->loadTexture(entry.path().string().c_str());
          if (textureId != -1) {
            loadedImageIds.push_back(textureId);
          }
        }
      }
    }
  }



public:


  void Load() {};


  void Init() {
    linearMonkeyImageId = Engine::GRAPHICS->loadTexture("Assets/images/monkey.png");
    nearestMonkeyImageId = Engine::GRAPHICS->loadTexture("Assets/images/monkey.png", Graphics::MinifyingFilter::Nearest);

    // Load all existing images from the assets folder on startup
    LoadAllAssets();

    std::cout << "Current path is " << std::filesystem::current_path() << std::endl;
  };

  void Update() {};

  void Draw() {

    //image stuff
    Engine::GRAPHICS->clearScreen(0.1f, 0.1f, 0.1f, 1.0f);
    Engine::GRAPHICS->drawTexture(linearMonkeyImageId, 200.0f, 150.0f, 150.0f, 210.0f);
    Engine::GRAPHICS->drawTexture(nearestMonkeyImageId, 400.0f, 150.0f, 150.0f, 210.0f);

    //limits the image to a set space
    Engine::GRAPHICS->drawTextureRegion(
      nearestMonkeyImageId, 600.0f, 150.0f, 150.0f, 210.0f, spritePosX += Engine::CORE->getDt() * 10.0f, 0.0f, 15.0f, 21.0f
    );

    //image menu
    ImGui::Begin("Image Menu");
    // The project root is now hardcoded, so we remove the button and associated logic.
    ImGui::Text("Project Root: %s", fs::current_path().string().c_str());
    ImGui::Separator();

    // The button that triggers the popup
    if (ImGui::Button("Browse for Image")) {
      showFileExplorerSource = true;
      fileCopied = false;
      selected_file_path.clear();
    }

    if (showFileExplorerSource) {
      FileExplorer(&showFileExplorerSource);
    }

    // Logic to copy the file after it's selected
    if (!showFileExplorerSource && !selected_file_path.empty() && !fileCopied) {
      try {
        // Construct the destination path correctly to be a file within the directory
        fs::path source_path(selected_file_path);
        fs::path destination_dir = fs::current_path() / "Engine" / "Assets" / "images";
        fs::path destination_path = destination_dir / source_path.filename();

        // Create the directory if it doesn't exist
        fs::create_directories(destination_dir);

        // Copy the file
        fs::copy_file(source_path, destination_path, fs::copy_options::overwrite_existing);

        std::cout << "File copied successfully to: " << destination_path << std::endl;

        // Reload all assets to include the new image
        LoadAllAssets();

        selected_file_path.clear();
        fileCopied = true;
      }
      catch (const fs::filesystem_error& e) {
        fprintf(stderr, "Filesystem error: %s\n", e.what());
      }
    }
    ImGui::End();

    //sprite dropper
    ImGui::Begin("Sprite Dropper");
    ImGui::Text("Current State: %s", typeid(*Engine::StateManager::GetCurrentState()).name());

    //put images into the drop space;
    ImGui::Text("Drop Images:");
    for (int imageId : loadedImageIds) {
      ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(imageId)), ImVec2(50, 50));
      ImGui::SameLine(); // places images on a same line
    }

    //if (ImGui::Button("Cool")) { Engine::StateManager::ChangeState(BasicState::Get()); }
    //if (ImGui::Button("Huh")) { Engine::StateManager::ChangeState(BasicState::Get()); }
    //if (ImGui::Button("Fweh")) { Engine::StateManager::ChangeState(BasicState::Get()); }
    ImGui::End();
  };

  void Free() {};
  void Unload() {};
};