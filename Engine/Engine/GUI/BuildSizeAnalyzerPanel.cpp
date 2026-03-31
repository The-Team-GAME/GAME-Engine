//-----------------------------------------------------------------------------------
// file:    BuildSizeAnalyzerPanel.cpp
// project: Monkey Stole My GPA
//
// brief:   Implementation of the Build Size Analyzer panel. Provides a full asset
//          list with checkboxes, level-based auto-detection of referenced assets,
//          persistent selection, and export to a user-chosen folder.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "BuildSizeAnalyzerPanel.h"
#include "../Precompiled.h"
#include "../Resources.h"
#include "../Path.h"
#include "../Logger.h"
#include "../FileIO/Settings.h"

#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <atomic>
#include <thread>
#include <mutex>

#ifdef _WIN32
#include <Windows.h>
#include <shlobj.h>
#endif

namespace {

  // -----------------------------------------------------------------------
  // Data types
  // -----------------------------------------------------------------------
  struct AssetEntry {
    std::string relative_path;
    size_t      file_size = 0;
  };

  // -----------------------------------------------------------------------
  // State
  // -----------------------------------------------------------------------
  std::vector<AssetEntry>          all_assets;
  std::unordered_set<std::string>  selected_assets;
  size_t                           total_selected_size = 0;
  int                              selected_file_count = 0;
  bool                             assets_loaded       = false;
  char                             search_filter[256]  = "";
  bool                             show_selected_only  = false;

  // Build + export state
  bool                             build_include_executable = false;
  std::atomic<bool>                build_in_progress{ false };
  std::string                      build_status_msg;
  std::mutex                       build_status_mutex;
  std::thread                      build_worker;

  // -----------------------------------------------------------------------
  // Helpers
  // -----------------------------------------------------------------------

  std::string format_file_size(size_t bytes) {
    if (bytes < 1024) {
      return std::to_string(bytes) + " B";
    }
    else if (bytes < 1024 * 1024) {
      char buf[64];
      snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
      return buf;
    }
    else if (bytes < 1024ULL * 1024 * 1024) {
      char buf[64];
      snprintf(buf, sizeof(buf), "%.2f MB", bytes / (1024.0 * 1024.0));
      return buf;
    }
    else {
      char buf[64];
      snprintf(buf, sizeof(buf), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
      return buf;
    }
  }

  // Normalise a path to forward-slash, no leading ./
  std::string normalize_relative(const std::string& p) {
    std::string out = p;
    std::replace(out.begin(), out.end(), '\\', '/');
    while (out.starts_with("./")) out = out.substr(2);
    return out;
  }

  std::string lowercase_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return value;
  }

  bool resource_asset_exists(const std::string& relative_path) {
    const std::string normalized = normalize_relative(relative_path);
    const std::string full_path = Engine::Path(Engine::Resources::resource_path).join(normalized).to_string();
    return std::filesystem::exists(full_path);
  }

  bool is_resource_managed_asset(const std::string& relative_path) {
    const std::string normalized = normalize_relative(relative_path);
    const std::string lower = lowercase_copy(normalized);

    auto dot = lower.rfind('.');
    if (dot == std::string::npos) {
      return false;
    }

    const std::string ext = lower.substr(dot);
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
        ext == ".tga" || ext == ".gif" || ext == ".ttf" || ext == ".otf") {
      return true;
    }

    if (ext == ".json") {
      return lower.starts_with("prefabs/") || lower.starts_with("levels/");
    }

    return false;
  }

  // Build export snapshot: selected assets + all non-resource-managed assets.
  std::unordered_set<std::string> build_export_snapshot(const std::unordered_set<std::string>& selected_snapshot) {
    std::unordered_set<std::string> export_snapshot = selected_snapshot;

    for (const auto& asset : all_assets) {
      if (!is_resource_managed_asset(asset.relative_path)) {
        export_snapshot.insert(asset.relative_path);
      }
    }

    return export_snapshot;
  }

  // -----------------------------------------------------------------------
  // Scanning
  // -----------------------------------------------------------------------

  void scan_all_assets() {
    all_assets.clear();
    const std::string& root = Engine::Resources::resource_path;
    if (root.empty() || !std::filesystem::exists(root))
      return;

    try {
      for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.is_directory()) continue;
        AssetEntry ae;
        ae.relative_path = normalize_relative(
          Engine::Path(entry.path().string()).to_relative(root));
        ae.file_size = entry.file_size();
        all_assets.push_back(std::move(ae));
      }
    }
    catch (const std::filesystem::filesystem_error& e) {
      Log::error("BuildSizeAnalyzer: failed to scan assets — {}", e.what());
    }

    std::sort(all_assets.begin(), all_assets.end(),
      [](const AssetEntry& a, const AssetEntry& b) { return a.relative_path < b.relative_path; });
  }

  void recalculate_total_size() {
    total_selected_size = 0;
    selected_file_count = 0;
    for (const auto& ae : all_assets) {
      if (selected_assets.contains(ae.relative_path)) {
        total_selected_size += ae.file_size;
        ++selected_file_count;
      }
    }
  }

  // -----------------------------------------------------------------------
  // Persistence
  // -----------------------------------------------------------------------

  void save_selection() {
    auto& ls = Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::get_mutable();
    ls.build_export_selected_assets.assign(selected_assets.begin(), selected_assets.end());
    Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::save();
  }

  void load_selection() {
    const auto& ls = Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::get();
    selected_assets.clear();
    selected_assets.insert(ls.build_export_selected_assets.begin(), ls.build_export_selected_assets.end());
    recalculate_total_size();
  }

  // -----------------------------------------------------------------------
  // Export + Build
  // -----------------------------------------------------------------------

#ifdef _WIN32
  std::optional<std::string> pick_folder_dialog() {
    BROWSEINFOA bi = {};
    bi.lpszTitle = "Select export destination folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (!pidl) return std::nullopt;

    char path[MAX_PATH] = {};
    if (SHGetPathFromIDListA(pidl, path)) {
      CoTaskMemFree(pidl);
      return std::string(path);
    }
    CoTaskMemFree(pidl);
    return std::nullopt;
  }
#else
  std::optional<std::string> pick_folder_dialog() {
    Log::error("BuildSizeAnalyzer: folder dialog not supported on this platform");
    return std::nullopt;
  }
#endif

  void set_build_status(const std::string& msg) {
    std::lock_guard<std::mutex> lk(build_status_mutex);
    build_status_msg = msg;
  }

  std::string get_build_status() {
    std::lock_guard<std::mutex> lk(build_status_mutex);
    return build_status_msg;
  }

  // Export a snapshot of assets from source_root to dest_folder.
  // asset_subdir: if non-empty, assets are placed under dest_folder/asset_subdir/<rel>.
  void do_export_snapshot(const std::unordered_set<std::string>& snapshot,
                          const std::string& source_root,
                          const std::string& dest_folder,
                          const std::string& asset_subdir = "") {
    int success_count = 0;
    int fail_count    = 0;

    for (const auto& rel : snapshot) {
      std::string src = Engine::Path(source_root).join(rel).to_string();
      std::string dst_rel = asset_subdir.empty() ? rel : (asset_subdir + "/" + rel);
      std::string dst = Engine::Path(dest_folder).join(dst_rel).to_string();

      try {
        std::filesystem::path dst_path(dst);
        std::filesystem::create_directories(dst_path.parent_path());
        std::filesystem::copy(src, dst, std::filesystem::copy_options::overwrite_existing);
        ++success_count;
      }
      catch (const std::filesystem::filesystem_error& e) {
        Log::error("BuildSizeAnalyzer: failed to export {} — {}", rel, e.what());
        ++fail_count;
      }
    }

    Log::info("BuildSizeAnalyzer: exported {} files ({} failed) to {}",
              success_count, fail_count, dest_folder);
  }

  // Returns the solution root (two levels above Assets/).
  std::string get_solution_root() {
    std::string rp = Engine::Resources::resource_path;
    while (!rp.empty() && (rp.back() == '/' || rp.back() == '\\')) {
      rp.pop_back();
    }

    namespace fs = std::filesystem;
    return fs::path(rp).parent_path().parent_path().string();
  }

  std::filesystem::path get_game_build_root(const std::string& solution_root) {
    return std::filesystem::path(solution_root) / "x64" / "Game";
  }

  int copy_runtime_outputs(const std::filesystem::path& source_root,
                           const std::filesystem::path& dest_root) {
    int copy_fails = 0;

    try {
      std::filesystem::create_directories(dest_root);

      for (const auto& entry : std::filesystem::directory_iterator(source_root)) {
        if (!entry.is_regular_file()) {
          continue;
        }

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".exe" && ext != ".dll") {
          continue;
        }

        try {
          std::filesystem::copy_file(entry.path(), dest_root / entry.path().filename(),
                                     std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::filesystem::filesystem_error& e) {
          Log::error("BuildSizeAnalyzer: failed to copy {} — {}",
                     entry.path().filename().string(), e.what());
          ++copy_fails;
        }
      }
    }
    catch (const std::filesystem::filesystem_error& e) {
      Log::error("BuildSizeAnalyzer: failed to enumerate runtime outputs — {}", e.what());
      return copy_fails + 1;
    }

    return copy_fails;
  }

  int copy_directory_tree(const std::filesystem::path& source_root,
                          const std::filesystem::path& dest_root) {
    if (!std::filesystem::exists(source_root)) {
      return 0;
    }

    int copy_fails = 0;

    try {
      std::filesystem::create_directories(dest_root);

      for (const auto& entry : std::filesystem::recursive_directory_iterator(source_root)) {
        const auto rel_path  = std::filesystem::relative(entry.path(), source_root);
        const auto dest_path = dest_root / rel_path;

        if (entry.is_directory()) {
          std::filesystem::create_directories(dest_path);
          continue;
        }

        try {
          std::filesystem::create_directories(dest_path.parent_path());
          std::filesystem::copy_file(entry.path(), dest_path,
                                     std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::filesystem::filesystem_error& e) {
          Log::error("BuildSizeAnalyzer: failed to copy {} — {}",
                     entry.path().string(), e.what());
          ++copy_fails;
        }
      }
    }
    catch (const std::filesystem::filesystem_error& e) {
      Log::error("BuildSizeAnalyzer: failed to copy directory {} — {}",
                 source_root.string(), e.what());
      return copy_fails + 1;
    }

    return copy_fails;
  }

  // Launches build_game.bat synchronously. Returns the process exit code, or -1 on failure.
  int run_build_bat(const std::string& solution_root) {
#ifdef _WIN32
    std::string bat = Engine::Path(solution_root).join("build_game.bat").to_string();
    std::replace(bat.begin(), bat.end(), '/', '\\');

    std::string cmd_line = "cmd.exe /c \"" + bat + "\"";

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessA(nullptr, cmd_line.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, solution_root.c_str(), &si, &pi)) {
      Log::error("BuildSizeAnalyzer: CreateProcess failed ({})", GetLastError());
      return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 1;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return static_cast<int>(exit_code);
#else
    return -1;
#endif
  }

  // Background worker: export selected assets (and optionally build the game first).
  void worker_export_and_build(const std::unordered_set<std::string> snapshot,
                               std::string dest_folder,
                               bool include_exe) {
    std::string assets_source = Engine::Resources::resource_path;

    if (include_exe) {
      set_build_status("Building game \xe2\x80\x94 this may take a minute...");
      const std::string solution_root = get_solution_root();
      const int build_result = run_build_bat(solution_root);

      if (build_result != 0) {
        set_build_status("Build FAILED (exit code " + std::to_string(build_result) +
                         "). Check the Output or build_game.bat output.");
        build_in_progress.store(false);
        return;
      }

      const std::filesystem::path game_build_root = get_game_build_root(solution_root);

      if (!std::filesystem::exists(game_build_root / "Engine.exe")) {
        set_build_status("Build completed, but x64/Game/Engine.exe was not found.");
        build_in_progress.store(false);
        return;
      }

      set_build_status("Copying game runtime files...");
      copy_runtime_outputs(game_build_root, std::filesystem::path(dest_folder));

      // Use the built assets as the copy source so patched settings are included.
      assets_source = (game_build_root / "Assets").string() + "/";
    }

    set_build_status("Exporting assets...");
    do_export_snapshot(snapshot, assets_source, dest_folder, "Assets");

    set_build_status("Done! Exported to: " + dest_folder);
    build_in_progress.store(false);
  }

} // anonymous namespace

// =========================================================================
// Panel render function
// =========================================================================

void BuildSizeAnalyzerPanel() {
  if (!ImGui::Begin("Build Size Analyzer")) {
    ImGui::End();
    return;
  }

  // --- Lazy init ---
  if (!assets_loaded) {
    scan_all_assets();
    load_selection();
    assets_loaded = true;
  }

  // =================================================================
  // Asset List
  // =================================================================
  const bool is_building = build_in_progress.load();

  if (ImGui::Button("Refresh")) {
    scan_all_assets();
    std::unordered_set<std::string> valid;
    for (const auto& ae : all_assets) {
      if (selected_assets.contains(ae.relative_path))
        valid.insert(ae.relative_path);
    }
    selected_assets = std::move(valid);
    recalculate_total_size();
    save_selection();
  }
  ImGui::SameLine();
  ImGui::Text("Total: %s across %d file(s)",
              format_file_size(total_selected_size).c_str(), selected_file_count);

  if (ImGui::Button("Select All")) {
    for (const auto& ae : all_assets) selected_assets.insert(ae.relative_path);
    recalculate_total_size();
    save_selection();
  }
  ImGui::SameLine();
  if (ImGui::Button("Deselect All")) {
    selected_assets.clear();
    recalculate_total_size();
    save_selection();
  }
  ImGui::SameLine();
  ImGui::Checkbox("Selected only", &show_selected_only);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(200.0f);
  ImGui::InputTextWithHint("##filter", "Filter assets...", search_filter, sizeof(search_filter));

  ImGui::Spacing();

  // Reserve space for the separator + export controls at the bottom
  const float footer_height = ImGui::GetFrameHeightWithSpacing() * 3.0f + 8.0f;
  if (ImGui::BeginTable("##assets_table", 3,
    ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
    ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp,
    ImVec2(0.0f, -footer_height)))
  {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("##check", ImGuiTableColumnFlags_WidthFixed, 30.0f);
    ImGui::TableSetupColumn("Asset Path", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Size",       ImGuiTableColumnFlags_WidthFixed, 90.0f);
    ImGui::TableHeadersRow();

    std::string filter_lower;
    if (search_filter[0] != '\0') {
      filter_lower = search_filter;
      std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), ::tolower);
    }

    for (const auto& ae : all_assets) {
      const bool is_selected = selected_assets.contains(ae.relative_path);
      if (show_selected_only && !is_selected) continue;

      if (!filter_lower.empty()) {
        std::string name_lower = ae.relative_path;
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
        if (name_lower.find(filter_lower) == std::string::npos) continue;
      }

      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      bool checked = is_selected;
      std::string id = "##" + ae.relative_path;
      if (ImGui::Checkbox(id.c_str(), &checked)) {
        if (checked) selected_assets.insert(ae.relative_path);
        else         selected_assets.erase(ae.relative_path);
        recalculate_total_size();
        save_selection();
      }

      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted(ae.relative_path.c_str());

      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(format_file_size(ae.file_size).c_str());
    }

    ImGui::EndTable();
  }

  // =================================================================
  // Section 3: Export / Build
  // =================================================================
  ImGui::Separator();

  // Option: include game executable
  ImGui::Checkbox("Build game executable (Game config)", &build_include_executable);
  if (build_include_executable) {
    ImGui::SameLine();
    ImGui::TextDisabled("  Builds game, then exports selected + unmanaged assets");
  }

  ImGui::Spacing();

  if (is_building) {
    ImGui::BeginDisabled();
    ImGui::Button("Working...");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::Text("%s", get_build_status().c_str());
  }
  else {
    const char* btn_label = build_include_executable
                            ? "Export Assets & Build Game..."
                            : "Export Selected Assets...";

    if (ImGui::Button(btn_label)) {
      if (selected_assets.empty() && !build_include_executable) {
        Log::info("BuildSizeAnalyzer: no assets selected for export");
        set_build_status("Nothing selected.");
      }
      else {
        auto folder = pick_folder_dialog();
        if (folder.has_value()) {
          // Join any previous worker before launching a new one
          if (build_worker.joinable()) build_worker.join();

          build_in_progress.store(true);
          set_build_status("Starting...");

          // Snapshot: selected assets + all unmanaged runtime assets
          auto snapshot     = build_export_snapshot(selected_assets);
          auto dest         = folder.value();
          bool inc_exe      = build_include_executable;

          build_worker = std::thread([snapshot, dest, inc_exe]() {
            worker_export_and_build(snapshot, dest, inc_exe);
          });
        }
      }
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(%d files, %s)", selected_file_count,
                        format_file_size(total_selected_size).c_str());

    // Show last status message (success / error from previous run)
    std::string status = get_build_status();
    if (!status.empty()) {
      ImGui::TextDisabled("%s", status.c_str());
    }
  }

  ImGui::End();
}
