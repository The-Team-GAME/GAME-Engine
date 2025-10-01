#include "FileExplorer.h"
#include <imgui.h>

std::string selected_file_path;
std::string root_path;

void FileExplorer(bool* p_open) {

  static fs::path current_path;
  static bool first_run = true;
  static std::stack<fs::path> history;


  if (first_run) {

    first_run = false;
    fs::path home_path;

    char* user_profile = nullptr;
    size_t size = 0;

    if (_dupenv_s(&user_profile, &size, "USERPROFILE") == 0 && user_profile != nullptr) {

      home_path = fs::path(user_profile) / "Downloads";
      free(user_profile);
    }


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

    // ensures only files accepted are .jpg .png .jpeg
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



    //sorts and remembers pathways
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