#pragma once

#include "StateManager.h"
#include "core.h"
#include "ObjectFactory.h"
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cstdlib> // For getenv
#include <stack>


namespace fs = std::filesystem;

// The file path of the image that was selected
static std::string selected_file_path;

//my file explorer function :>
void FileExplorer(bool* p_open) {
  /*
   * Static variables maintain state across multiple calls (frames).
   * This avoids having to pass state data in and out of the function.
   */
  static fs::path current_path;
  static bool first_run = true;
  static std::stack<fs::path> history;

  // This block runs only once when the function is first called.
  if (first_run) {
    first_run = false;

    fs::path home_path;

    // Use a platform-specific approach to find the user's Downloads directory.
#ifdef _WIN32
        // For Windows, use the secure _dupenv_s function to get the USERPROFILE environment variable.
    char* user_profile = nullptr;
    size_t size = 0;
    if (_dupenv_s(&user_profile, &size, "USERPROFILE") == 0 && user_profile != nullptr) {
      home_path = fs::path(user_profile) / "Downloads";
      free(user_profile); // Important: free the memory allocated by _dupenv_s.
    }
#else
        // For Linux/macOS, use the standard getenv function.
    const char* home_dir = getenv("HOME");
    if (home_dir) {
      home_path = fs::path(home_dir) / "Downloads";
    }
#endif

    // Fallback to the current working directory if the Downloads folder path cannot be found.
    if (home_path.empty()) {
      current_path = fs::current_path();
    }
    else {
      current_path = home_path;
    }
  }

  // Begin a new ImGui window with the title "File Explorer".
  if (ImGui::Begin("File Explorer", p_open)) {

    // Create a "Back" button that navigates to the previous directory from the history stack.
    if (!history.empty() && ImGui::Button("Back")) {
      current_path = history.top();
      history.pop();
    }
    ImGui::SameLine();

    // Create a ".." button to navigate up one directory level.
    if (ImGui::Button("..")) {
      if (current_path.has_parent_path()) {
        history.push(current_path); // Push the current path to history before changing.
        current_path = current_path.parent_path();
      }
    }

    ImGui::Separator();

    // Display an error message if the current path is invalid or not a directory.
    if (!fs::exists(current_path) || !fs::is_directory(current_path)) {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Invalid directory path!");
      ImGui::End();
      return;
    }

    // List files and directories in the current path.
    std::vector<fs::directory_entry> entries;
    for (const auto& entry : fs::directory_iterator(current_path)) {
      // Filter to only show directories and specific image file types.
      if (fs::is_directory(entry.path())) {
        entries.push_back(entry);
      }
      else {
        std::string extension = entry.path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        // The accepted extensions are .jpg, .png, and .jpeg.
        if (extension == ".jpg" || extension == ".png" || extension == ".jpeg") {
          entries.push_back(entry);
        }
      }
    }

    // Sort the entries: directories first, then files, both alphabetically.
    std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
      bool a_is_dir = fs::is_directory(a.path());
      bool b_is_dir = fs::is_directory(b.path());
      if (a_is_dir && !b_is_dir) { return true; }
      if (!a_is_dir && b_is_dir) { return false; }
      return a.path().filename().string() < b.path().filename().string();
      });

    // Display each entry as a clickable item.
    for (const auto& entry : entries) {
      bool is_directory = fs::is_directory(entry.path());
      const char* icon = is_directory ? "[DIR]" : "[FILE]";

      // ImGui::Selectable creates a clickable item.
      if (ImGui::Selectable((icon + std::string(" ") + entry.path().filename().string()).c_str(), false, 0, ImVec2(0, 20))) {
        if (is_directory) {
          // If a directory is clicked, navigate into it and save the current path to history.
          history.push(current_path);
          current_path /= entry.path().filename();
        }
        else {
          // If a file is clicked, the selected_file_path is updated, and the window closes.
          selected_file_path = entry.path().string();
          *p_open = false;
        }
      }
    }
  }
  // End the ImGui window.
  ImGui::End();
}



class SpriteState : public Engine::GameState<SpriteState>
{
private:
  int linearMonkeyImageId = -1;
  int nearestMonkeyImageId = -1;
  bool showFileExplorer = false;


  struct ImageData{
    unsigned int texture_id;
    const char* name;
  };
  std::vector<ImageData> ImageAspectRatio;

  float spritePosX = 0.0f;

public:
  void Load() {};

  void Init()
  {
    linearMonkeyImageId = Engine::GRAPHICS->loadTexture("Assets/images/monkey.png");
    nearestMonkeyImageId = Engine::GRAPHICS->loadTexture("Assets/images/monkey.png", Graphics::MinifyingFilter::Nearest); // when loading pixel art, use nearest filter to avoid blurriness
  };

  void Update() {};

  void Draw()
  {
    Engine::GRAPHICS->clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background

    //drawing monkey
    Engine::GRAPHICS->drawTexture(linearMonkeyImageId, 200.0f, 150.0f, 150.0f, 210.0f);
    Engine::GRAPHICS->drawTexture(nearestMonkeyImageId, 400.0f, 150.0f, 150.0f, 210.0f);

    // Draw a portion of the texture (sprite) to animate the sprite
    Engine::GRAPHICS->drawTextureRegion(
      nearestMonkeyImageId,
      600.0f, // destX
      150.0f, // destY
      150.0f, // destWidth
      210.0f, // destHeight
      spritePosX += Engine::CORE->getDt() * 10.0f, // srcX
      0.0f, // srcY
      15.0f, // srcWidth
      21.0f // srcHeight
    );



    //IMGUI STUFF

    //Image menu
    ImGui::Begin("Image Menu");

    //button to input image
    if (ImGui::Button("Browse...")) {
      showFileExplorer = true; // Set the boolean to true when the button is pressed
    }

    ImGui::End();

    // Call the file explorer function conditionally and pass the address of the boolean
    if (showFileExplorer) {
      FileExplorer(&showFileExplorer);
    }



    ImGui::Begin("Sprite Dropper");

    ImGui::Text("Current State: %s", typeid(*Engine::StateManager::GetCurrentState()).name());

    if (ImGui::Button("Cool"))
    {
      Engine::StateManager::ChangeState(BasicState::Get());
    }
    if (ImGui::Button("Huh"))
    {
      Engine::StateManager::ChangeState(BasicState::Get());
    }
    if (ImGui::Button("Fweh"))
    {
      Engine::StateManager::ChangeState(BasicState::Get());
    }


    ImGui::End();
  };

  void Free() {};

  void Unload() {};
};