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
#include "../FileIO/Helpers.h"
#include "../FileIO/Settings.h"

#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>#include <atomic>
#include <thread>
#include <mutex>

#include <rapidjson/document.h>

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
  std::vector<std::string>         available_levels;
  std::unordered_set<std::string>  selected_levels;
  size_t                           total_selected_size = 0;
  int                              selected_file_count = 0;
  bool                             assets_loaded       = false;
  char                             search_filter[256]  = "";

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

  void scan_available_levels() {
    available_levels.clear();
    const std::string levels_dir = Engine::Path(Engine::Resources::resource_path).join("levels").to_string();
    if (!std::filesystem::exists(levels_dir)) return;

    try {
      for (const auto& entry : std::filesystem::directory_iterator(levels_dir)) {
        if (entry.is_directory()) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".json") {
          available_levels.push_back(
            normalize_relative(Engine::Path(entry.path().string()).to_relative(Engine::Resources::resource_path)));
        }
      }
    }
    catch (const std::filesystem::filesystem_error& e) {
      Log::error("BuildSizeAnalyzer: failed to scan levels — {}", e.what());
    }
    std::sort(available_levels.begin(), available_levels.end());
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
    ls.build_analyzer_selected_levels.assign(selected_levels.begin(), selected_levels.end());
    Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::save();
  }

  void load_selection() {
    const auto& ls = Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::get();
    selected_assets.clear();
    selected_assets.insert(ls.build_export_selected_assets.begin(), ls.build_export_selected_assets.end());
    selected_levels.clear();
    selected_levels.insert(ls.build_analyzer_selected_levels.begin(), ls.build_analyzer_selected_levels.end());
    recalculate_total_size();
  }

  // -----------------------------------------------------------------------
  // JSON dependency scanner
  // -----------------------------------------------------------------------

  bool has_asset_extension(const std::string& s) {
    auto dot = s.rfind('.');
    if (dot == std::string::npos) return false;
    std::string ext = s.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
           ext == ".tga" || ext == ".gif" || ext == ".ttf" || ext == ".otf" ||
           ext == ".lua" || ext == ".json" || ext == ".wav" || ext == ".ogg" ||
           ext == ".mp3" || ext == ".mpeg" || ext == ".mpg" || ext == ".glsl" ||
           ext == ".vert" || ext == ".frag";
  }

  void collect_paths_from_json(const rapidjson::Value& val, std::unordered_set<std::string>& out) {
    if (val.IsString()) {
      std::string s = val.GetString();
      // Skip FMOD event paths and empty strings
      if (s.empty() || s.starts_with("event:/")) return;
      // Normalise and check
      s = normalize_relative(s);
      if (s.find('/') != std::string::npos && has_asset_extension(s)) {
        out.insert(s);
      }
    }
    else if (val.IsObject()) {
      for (auto it = val.MemberBegin(); it != val.MemberEnd(); ++it) {
        collect_paths_from_json(it->value, out);
      }
    }
    else if (val.IsArray()) {
      for (rapidjson::SizeType i = 0; i < val.Size(); ++i) {
        collect_paths_from_json(val[i], out);
      }
    }
  }

  std::unordered_set<std::string> scan_level_for_assets(const std::string& level_relative_path) {
    std::unordered_set<std::string> result;
    const std::string abs_path = Engine::Path(Engine::Resources::resource_path).join(level_relative_path).to_string();

    auto contents = Engine::FileIO::read_string_from_file(abs_path);
    if (!contents.has_value()) return result;

    rapidjson::Document doc;
    doc.Parse(contents->c_str());
    if (doc.HasParseError()) {
      Log::error("BuildSizeAnalyzer: JSON parse error in {}", level_relative_path);
      return result;
    }

    // Collect all string paths from the level JSON
    collect_paths_from_json(doc, result);

    // One level of prefab expansion: for each prefab/*.json found, scan it too
    std::vector<std::string> prefabs_to_scan;
    for (const auto& p : result) {
      std::string lower = p;
      std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
      if (lower.starts_with("prefabs/") && lower.ends_with(".json")) {
        prefabs_to_scan.push_back(p);
      }
    }

    for (const auto& prefab_path : prefabs_to_scan) {
      std::string prefab_abs = Engine::Path(Engine::Resources::resource_path).join(prefab_path).to_string();
      auto prefab_contents = Engine::FileIO::read_string_from_file(prefab_abs);
      if (!prefab_contents.has_value()) continue;

      rapidjson::Document prefab_doc;
      prefab_doc.Parse(prefab_contents->c_str());
      if (prefab_doc.HasParseError()) continue;

      collect_paths_from_json(prefab_doc, result);
    }

    // Validate that each discovered path actually exists on disk
    std::unordered_set<std::string> validated;
    for (const auto& p : result) {
      std::string full = Engine::Path(Engine::Resources::resource_path).join(p).to_string();
      if (std::filesystem::exists(full)) {
        validated.insert(p);
      }
    }
    return validated;
  }

  std::vector<std::string> get_fmod_bank_paths() {
    std::vector<std::string> banks;
    std::string fmod_dir = Engine::Path(Engine::Resources::resource_path).join("fmodstudio").to_string();
    if (!std::filesystem::exists(fmod_dir)) return banks;

    try {
      for (const auto& entry : std::filesystem::recursive_directory_iterator(fmod_dir)) {
        if (entry.is_directory()) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".bank" || entry.path().string().ends_with(".strings.bank")) {
          banks.push_back(normalize_relative(
            Engine::Path(entry.path().string()).to_relative(Engine::Resources::resource_path)));
        }
      }
    }
    catch (const std::filesystem::filesystem_error&) {}
    return banks;
  }

  void auto_detect_from_levels() {
    selected_assets.clear();

    for (const auto& level : selected_levels) {
      // Add the level file itself
      selected_assets.insert(level);

      // Scan the level JSON for asset references
      auto deps = scan_level_for_assets(level);
      selected_assets.insert(deps.begin(), deps.end());
    }

    // Always include all FMOD bank files
    auto banks = get_fmod_bank_paths();
    selected_assets.insert(banks.begin(), banks.end());

    recalculate_total_size();
    save_selection();
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

  // Export a snapshot of assets to dest_folder.
  // asset_subdir: if non-empty, assets are placed under dest_folder/asset_subdir/<rel>.
  void do_export_snapshot(const std::unordered_set<std::string>& snapshot,
                          const std::string& dest_folder,
                          const std::string& asset_subdir = "") {
    int success_count = 0;
    int fail_count    = 0;

    for (const auto& rel : snapshot) {
      std::string src = Engine::Path(Engine::Resources::resource_path).join(rel).to_string();
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
    // Strip trailing slash(es)
    while (!rp.empty() && (rp.back() == '/' || rp.back() == '\\'))
      rp.pop_back();
    // rp is now .../Engine/Assets — go up twice
    namespace fs = std::filesystem;
    return fs::path(rp).parent_path().parent_path().string();
  }

  // Launches build_game.bat synchronously. Returns the process exit code, or -1 on failure.
  int run_build_bat(const std::string& solution_root) {
#ifdef _WIN32
    std::string bat = Engine::Path(solution_root).join("build_game.bat").to_string();
    std::replace(bat.begin(), bat.end(), '/', '\\');

    // cmd /c "path\build_game.bat"
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

  // Background worker: export assets (optionally into Assets/ subdir), build game, copy binaries.
  void worker_export_and_build(const std::unordered_set<std::string> snapshot,
                               std::string dest_folder,
                               bool include_exe) {
    // --- Step 1: Export assets ---
    set_build_status("Exporting assets...");
    const std::string subdir = include_exe ? "Assets" : "";
    do_export_snapshot(snapshot, dest_folder, subdir);

    if (!include_exe) {
      set_build_status("Export complete.");
      build_in_progress.store(false);
      return;
    }

    // --- Step 2: Build game ---
    set_build_status("Building game — this may take a minute...");
    const std::string solution_root = get_solution_root();
    const int build_result = run_build_bat(solution_root);

    if (build_result != 0) {
      set_build_status("Build FAILED (exit code " + std::to_string(build_result) +
                       "). Check the Output or build_game.bat output.");
      build_in_progress.store(false);
      return;
    }

    // --- Step 3: Copy exe + DLLs ---
    set_build_status("Copying game executable and DLLs...");
    static const std::vector<std::string> game_outputs = {
      "x64/Game/Engine.exe",
      "x64/Game/fmod.dll",
      "x64/Game/fmodstudio.dll",
      "x64/Game/lua54.dll",
    };

    namespace fs = std::filesystem;
    int copy_fails = 0;
    for (const auto& rel : game_outputs) {
      std::string src = Engine::Path(solution_root).join(rel).to_string();
      std::string filename = fs::path(src).filename().string();
      std::string dst = Engine::Path(dest_folder).join(filename).to_string();

      if (!fs::exists(src)) {
        Log::error("BuildSizeAnalyzer: expected build output not found: {}", src);
        ++copy_fails;
        continue;
      }
      try {
        fs::copy(src, dst, fs::copy_options::overwrite_existing);
      }
      catch (const fs::filesystem_error& e) {
        Log::error("BuildSizeAnalyzer: failed to copy {} — {}", filename, e.what());
        ++copy_fails;
      }
    }

    if (copy_fails == 0) {
      set_build_status("Done! Game built and exported to: " + dest_folder);
    }
    else {
      set_build_status("Done with " + std::to_string(copy_fails) +
                       " copy error(s). Check the log for details.");
    }
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
    scan_available_levels();
    load_selection();
    assets_loaded = true;
  }

  // =================================================================
  // Section 1: Level Analysis
  // =================================================================
  if (ImGui::CollapsingHeader("Level Analysis", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::TextWrapped("Select levels to auto-detect their referenced assets:");
    ImGui::Spacing();

    for (const auto& level : available_levels) {
      bool checked = selected_levels.contains(level);
      if (ImGui::Checkbox(level.c_str(), &checked)) {
        if (checked) selected_levels.insert(level);
        else         selected_levels.erase(level);
        save_selection();
      }
    }
    if (available_levels.empty()) {
      ImGui::TextDisabled("No level files found in levels/");
    }

    ImGui::Spacing();
    if (ImGui::Button("Auto-Select from Levels")) {
      if (!selected_levels.empty()) {
        auto_detect_from_levels();
      }
    }
    if (!selected_levels.empty()) {
      ImGui::SameLine();
      ImGui::Text("(%zu level(s) chosen)", selected_levels.size());
    }
    ImGui::Separator();
  }

  // =================================================================
  // Section 2: Asset List
  // =================================================================
  const bool is_building = build_in_progress.load();

  if (ImGui::Button("Refresh")) {
    scan_all_assets();
    scan_available_levels();
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
      if (!filter_lower.empty()) {
        std::string name_lower = ae.relative_path;
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
        if (name_lower.find(filter_lower) == std::string::npos) continue;
      }

      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      bool checked = selected_assets.contains(ae.relative_path);
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
    ImGui::TextDisabled("  Assets -> dest/Assets/   |   Engine.exe + DLLs -> dest/");
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

          // Snapshot current selection so the thread has its own copy
          auto snapshot     = selected_assets;
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
