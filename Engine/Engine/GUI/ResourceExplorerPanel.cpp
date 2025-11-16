//-----------------------------------------------------------------------------------
// file:    ResourceExplorerPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of ResourceExplorerPanel for browsing and managing
//          resources in the resource folder. Provides file explorer functionality
//          with previews and resource loading capabilities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ResourceExplorerPanel.h"
#include <imgui.h>
#include "../Resources.h"
#include "../Path.h"
#include "../Logger.h"
#include "../FileIO/LevelFileIO.h"
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>

namespace {
  // Current directory path (relative to resource root)
  std::string current_directory = "";

  // Selected file/folder
  std::string selected_item = "";

  // Preview texture for selected image
  Engine::Graphics::Texture preview_texture;

  // File filter
  char search_filter[256] = "";

  struct FileEntry {
    std::string name;
    std::string path;
    bool is_directory;
    size_t file_size;
    std::string extension;
  };

  std::vector<FileEntry> get_directory_contents(const std::string& dir_path) {
    std::vector<FileEntry> entries;

    std::string full_path = Engine::Path(Engine::Resources::resource_path).join(dir_path).to_string();

    if (!std::filesystem::exists(full_path)) {
      return entries;
    }

    try {
      for (const auto& entry : std::filesystem::directory_iterator(full_path)) {
        FileEntry file_entry;
        file_entry.name = entry.path().filename().string();
        file_entry.path = Engine::Path(entry.path().string()).to_relative(Engine::Resources::resource_path);
        file_entry.is_directory = entry.is_directory();
        file_entry.file_size = entry.is_regular_file() ? entry.file_size() : 0;
        file_entry.extension = entry.is_regular_file() ? Engine::Path(file_entry.name).get_extension() : "";

        // Filter by search
        if (search_filter[0] != '\0') {
          std::string search_lower = search_filter;
          std::string name_lower = file_entry.name;
          std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
          std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

          if (name_lower.find(search_lower) == std::string::npos) {
            continue;
          }
        }

        entries.push_back(file_entry);
      }
    }
    catch (const std::filesystem::filesystem_error& e) {
      Log::error("Failed to read directory: " + std::string(e.what()));
    }

    // Sort: directories first, then by name
    std::sort(entries.begin(), entries.end(), [](const FileEntry& a, const FileEntry& b) {
      if (a.is_directory != b.is_directory) {
        return a.is_directory;
      }
      return a.name < b.name;
      });

    return entries;
  }

  std::string format_file_size(size_t bytes) {
    if (bytes < 1024) {
      return std::to_string(bytes) + " B";
    }
    else if (bytes < 1024 * 1024) {
      return std::to_string(bytes / 1024) + " KB";
    }
    else {
      return std::to_string(bytes / (1024 * 1024)) + " MB";
    }
  }

  bool is_image_file(const std::string& extension) {
    std::string ext_lower = extension;
    std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);

    return ext_lower == ".png" || ext_lower == ".jpg" || ext_lower == ".jpeg" ||
      ext_lower == ".bmp" || ext_lower == ".tga" || ext_lower == ".gif";
  }

  void load_preview_for_selected() {
    if (selected_item.empty()) {
      preview_texture = nullptr;
      return;
    }

    std::string full_path = Engine::Path(Engine::Resources::resource_path).join(selected_item).to_string();

    if (!std::filesystem::exists(full_path) || std::filesystem::is_directory(full_path)) {
      preview_texture = nullptr;
      return;
    }

    std::string extension = Engine::Path(selected_item).get_extension();

    if (is_image_file(extension)) {
      preview_texture = Engine::Resources::load_texture(selected_item, true, false);
    }
    else {
      preview_texture = nullptr;
    }
  }
}

void ResourceExplorerPanel() {
  ImGui::Begin("Resource Explorer");

  // Breadcrumb navigation
  ImGui::Text("Path: ");
  ImGui::SameLine();

  if (ImGui::Button("Root")) {
    current_directory = "";
    selected_item = "";
    preview_texture = nullptr;
  }

  // Build breadcrumb trail
  if (!current_directory.empty()) {
    std::vector<std::string> path_parts;
    std::string temp_path = current_directory;

    size_t pos = 0;
    while ((pos = temp_path.find('/')) != std::string::npos) {
      path_parts.push_back(temp_path.substr(0, pos));
      temp_path.erase(0, pos + 1);
    }
    if (!temp_path.empty()) {
      path_parts.push_back(temp_path);
    }

    std::string accumulated_path = "";
    for (size_t i = 0; i < path_parts.size(); ++i) {
      ImGui::SameLine();
      ImGui::Text("/");
      ImGui::SameLine();

      std::string button_label = path_parts[i] + "##breadcrumb_" + std::to_string(i);
      if (ImGui::Button(button_label.c_str())) {
        // Navigate to this path level
        accumulated_path = accumulated_path.empty() ? path_parts[i] : accumulated_path + "/" + path_parts[i];
        current_directory = accumulated_path;
        selected_item = "";
        preview_texture = nullptr;
        break;
      }

      accumulated_path = accumulated_path.empty() ? path_parts[i] : accumulated_path + "/" + path_parts[i];
    }
  }

  ImGui::Separator();

  // Toolbar
  if (ImGui::Button("New Folder")) {
    ImGui::OpenPopup("CreateFolder");
  }

  ImGui::SameLine();

  if (ImGui::Button("New File")) {
    ImGui::OpenPopup("CreateFile");
  }

  ImGui::SameLine();

  if (ImGui::Button("Refresh")) {
    // Just triggers a re-read on next frame
  }

  ImGui::SameLine();
  ImGui::InputText("Search", search_filter, sizeof(search_filter));

  // Create folder popup
  static char new_folder_name[128] = "NewFolder";
  if (ImGui::BeginPopupModal("CreateFolder", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Create new folder in current directory");
    ImGui::InputText("Folder Name", new_folder_name, sizeof(new_folder_name));

    if (ImGui::Button("Create", ImVec2(120, 0))) {
      std::string new_folder_path = current_directory.empty() ?
        new_folder_name :
        Engine::Path(current_directory).join(new_folder_name).to_string();

      std::string full_path = Engine::Path(Engine::Resources::resource_path).join(new_folder_path).to_string();

      try {
        std::filesystem::create_directory(full_path);
        Log::info("Created folder: " + new_folder_path);
      }
      catch (const std::filesystem::filesystem_error& e) {
        Log::error("Failed to create folder: " + std::string(e.what()));
      }

      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }

  // Create file popup
  static char new_file_name[128] = "NewFile";
  static int selected_file_type = 0; // 0 = Level
  if (ImGui::BeginPopupModal("CreateFile", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Create new file in current directory");

    ImGui::Separator();
    ImGui::Text("File Type:");
    ImGui::RadioButton("Level (.json)", &selected_file_type, 0);
    // Add more file types here in the future

    ImGui::Separator();
    ImGui::InputText("File Name", new_file_name, sizeof(new_file_name));

    if (ImGui::Button("Create", ImVec2(120, 0))) {
      std::string filename = std::string(new_file_name);

      // Handle different file types
      if (selected_file_type == 0) { // Level
        // Add .json extension if not present
        if (Engine::Path(filename).get_extension() != ".json") {
          filename += ".json";
        }

        std::string file_path = current_directory.empty() ?
          filename :
          Engine::Path(current_directory).join(filename).to_string();

        std::string full_path = Engine::Path(Engine::Resources::resource_path).join(file_path).to_string();

        // Create an empty level
        Engine::Level empty_level;

        if (Engine::FileIO::serialize_level_to_json_file(full_path, empty_level)) {
          Log::info("Created level: " + file_path);
        }
        else {
          Log::error("Failed to create level: " + file_path);
        }
      }

      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }

  ImGui::Separator();

  // Main content area - split into file list and preview
  ImGui::BeginChild("FileListPane", ImVec2(ImGui::GetContentRegionAvail().x * 0.6f, 0), true);

  // Get directory contents
  auto entries = get_directory_contents(current_directory);

  // Display parent directory option
  if (!current_directory.empty()) {
    if (ImGui::Selectable(".. (Parent Directory)", false, ImGuiSelectableFlags_AllowDoubleClick)) {
      if (ImGui::IsMouseDoubleClicked(0)) {
        // Navigate to parent
        size_t last_slash = current_directory.find_last_of('/');
        if (last_slash != std::string::npos) {
          current_directory = current_directory.substr(0, last_slash);
        }
        else {
          current_directory = "";
        }
        selected_item = "";
        preview_texture = nullptr;
      }
    }
  }

  // Display entries
  for (const auto& entry : entries) {
    bool is_selected = (entry.path == selected_item);

    std::string display_name = entry.is_directory ?
      "[DIR] " + entry.name :
      entry.name;

    if (!entry.is_directory) {
      display_name += " (" + format_file_size(entry.file_size) + ")";
    }

    ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick;

    if (ImGui::Selectable(display_name.c_str(), is_selected, flags)) {
      selected_item = entry.path;
      load_preview_for_selected();

      if (ImGui::IsMouseDoubleClicked(0)) {
        if (entry.is_directory) {
          // Navigate into directory
          current_directory = entry.path;
          selected_item = "";
          preview_texture = nullptr;
        }
        else {
          // Try to open the resource (e.g., instantiate level)
          if (!Engine::Resources::open(entry.path, true)) {
            // If open fails or is not supported, fall back to loading
            Engine::Resources::load(entry.path, true, false);
            Log::info("Loaded resource: " + entry.path);
          }
        }
      }
    }

    // Right-click context menu
    if (ImGui::BeginPopupContextItem()) {
      if (entry.is_directory) {
        if (ImGui::MenuItem("Open")) {
          current_directory = entry.path;
          selected_item = "";
          preview_texture = nullptr;
        }
      }
      else {
        if (ImGui::MenuItem("Open")) {
          if (!Engine::Resources::open(entry.path, true)) {
            Log::warning("Cannot open this resource type");
          }
        }

        if (ImGui::MenuItem("Load Resource")) {
          Engine::Resources::load(entry.path, true, false);
          Log::info("Loaded resource: " + entry.path);
        }

        if (ImGui::MenuItem("Load Resource (Persistent)")) {
          Engine::Resources::load(entry.path, true, true);
          Log::info("Loaded resource (persistent): " + entry.path);
        }
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Delete")) {
        std::string full_path = Engine::Path(Engine::Resources::resource_path).join(entry.path).to_string();
        try {
          if (entry.is_directory) {
            std::filesystem::remove_all(full_path);
          }
          else {
            std::filesystem::remove(full_path);
          }
          Log::info("Deleted: " + entry.path);

          if (selected_item == entry.path) {
            selected_item = "";
            preview_texture = nullptr;
          }
        }
        catch (const std::filesystem::filesystem_error& e) {
          Log::error("Failed to delete: " + std::string(e.what()));
        }
      }

      ImGui::EndPopup();
    }
  }

  ImGui::EndChild();

  // Preview pane
  ImGui::SameLine();

  ImGui::BeginChild("PreviewPane", ImVec2(0, 0), true);

  if (!selected_item.empty()) {
    std::string full_path = Engine::Path(Engine::Resources::resource_path).join(selected_item).to_string();

    if (std::filesystem::exists(full_path)) {
      ImGui::Text("Selected: %s", Engine::Path(selected_item).get_filename().c_str());
      ImGui::Separator();

      if (std::filesystem::is_directory(full_path)) {
        ImGui::Text("Type: Directory");

        // Count items in directory
        try {
          size_t item_count = 0;
          for (const auto& entry [[maybe_unused]] : std::filesystem::directory_iterator(full_path)) {

            item_count++;
          }
          ImGui::Text("Items: %zu", item_count);
        }
        catch (...) {
          ImGui::Text("Items: Unknown");
        }
      }
      else {
        ImGui::Text("Type: File");
        ImGui::Text("Size: %s", format_file_size(std::filesystem::file_size(full_path)).c_str());

        std::string extension = Engine::Path(selected_item).get_extension();
        ImGui::Text("Extension: %s", extension.c_str());

        ImGui::Separator();

        // Preview for images
        if (preview_texture && preview_texture->id != 0) {
          ImGui::Text("Preview:");

          // Calculate preview size maintaining aspect ratio
          float preview_width = ImGui::GetContentRegionAvail().x - 20;
          float aspect_ratio = static_cast<float>(preview_texture->width) / static_cast<float>(preview_texture->height);
          float preview_height = preview_width / aspect_ratio;

          // Cap height
          if (preview_height > 400) {
            preview_height = 400;
            preview_width = preview_height * aspect_ratio;
          }

          ImGui::Image(
            (ImTextureID)(intptr_t)preview_texture->id,
            ImVec2(preview_width, preview_height)
          );

          ImGui::Text("Dimensions: %d x %d", preview_texture->width, preview_texture->height);
        }
      }

      ImGui::Separator();
      ImGui::Text("Full Path:");
      ImGui::TextWrapped("%s", full_path.c_str());
    }
  }
  else {
    ImGui::Text("No item selected");
    ImGui::Separator();
    ImGui::TextWrapped("Select a file or folder to view details and preview.");
  }

  ImGui::EndChild();

  ImGui::End();
}
