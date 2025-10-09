//-----------------------------------------------------------------------------------
// file:    FileExplorer.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file implements the FileExplorer utility function. It provides a
//          graphical interface for the user to browse the filesystem, primarily
//          to select image files (.jpg, .png, .jpeg). It handles directory traversal,
//          path history, and filtering to return the selected file path.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "FileExplorer.h"
#include <imgui.h>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <stack>

namespace fs = std::filesystem;

// Global variables used for communication back to the calling state (SpriteState)
std::string selected_file_path;
std::string root_path;

/*=================================================================================
    Function Brief:
    Renders an ImGui window allowing the user to browse the local filesystem.
    It defaults to the user's Downloads folder, maintains a history stack for
    navigation, and filters files to show only directories and supported image formats.
    Upon selection, it sets the global `selected_file_path` and closes the window.

    Where to call it:
    Must be called inside the game state's main Draw() function whenever a file
    browsing dialogue is required, controlled by a boolean flag (p_open).
    ==================================================================================*/
void FileExplorer(bool* p_open) {

  static fs::path current_path;
  static bool first_run = true;
  static std::stack<fs::path> history;


  if (first_run) {

    first_run = false;
    fs::path home_path;

    char* user_profile = nullptr;
    size_t size = 0;

    // Use _dupenv_s for safe environment variable access (Windows specific)
    if (_dupenv_s(&user_profile, &size, "USERPROFILE") == 0 && user_profile != nullptr) {

      home_path = fs::path(user_profile) / "Downloads";
      free(user_profile);
    }

    // Fallback or use determined home path
    if (home_path.empty() || !fs::exists(home_path)) {
      current_path = fs::current_path();
    }

    else {
      current_path = home_path;
    }
  }

  if (ImGui::Begin("File Explorer", p_open)) {

    // Back button (uses history stack)
    if (!history.empty() && ImGui::Button("Back")) {
      current_path = history.top();
      history.pop();
    }

    ImGui::SameLine();


    // Go to Parent Directory button (like 'cd ..')
    // NOTE: The previous comment said "goes to desktop", but '..' goes up one level.
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

    // Gather and filter directory entries
    std::vector<fs::directory_entry> entries;

    for (const auto& entry : fs::directory_iterator(current_path)) {

      if (fs::is_directory(entry.path())) {
        entries.push_back(entry);
      }

      else {
        std::string extension = entry.path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        // Check for valid image extensions
        if (extension == ".jpg" || extension == ".png" || extension == ".jpeg") {
          entries.push_back(entry);
        }
      }
    }


    // Sorts: Directories first, then files alphabetically
    std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {

      bool a_is_dir = fs::is_directory(a.path());
      bool b_is_dir = fs::is_directory(b.path());
      if (a_is_dir && !b_is_dir) { return true; }
      if (!a_is_dir && b_is_dir) { return false; }
      return a.path().filename().string() < b.path().filename().string();
      });

    // Display entries
    for (const auto& entry : entries) {

      bool is_directory = fs::is_directory(entry.path());

      const char* icon = is_directory ? "[DIR]" : "[FILE]";

      if (ImGui::Selectable((icon + std::string(" ") + entry.path().filename().string()).c_str(), false, 0, ImVec2(0, 20))) {

        if (is_directory) {
          // Navigate into a directory
          history.push(current_path);
          current_path /= entry.path().filename();
        }

        else {
          // Select a file and close the explorer
          selected_file_path = entry.path().string();
          *p_open = false;
        }
      }
    }
  }
  ImGui::End();
}