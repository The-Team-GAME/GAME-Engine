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
#include <unordered_set>

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
  // Export
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

  void do_export(const std::string& dest_folder) {
    int success_count = 0;
    int fail_count = 0;

    for (const auto& rel : selected_assets) {
      std::string src = Engine::Path(Engine::Resources::resource_path).join(rel).to_string();
      std::string dst = Engine::Path(dest_folder).join(rel).to_string();

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

    Log::info("BuildSizeAnalyzer: exported {} files ({} failed) to {}", success_count, fail_count, dest_folder);
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

    // Level checkboxes
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

  // Toolbar
  if (ImGui::Button("Refresh")) {
    scan_all_assets();
    scan_available_levels();
    // Re-validate selection against actual files
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
  ImGui::Text("Total: %s across %d file(s)", format_file_size(total_selected_size).c_str(), selected_file_count);

  // Select All / Deselect All
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

  // Search filter
  ImGui::SameLine();
  ImGui::SetNextItemWidth(200.0f);
  ImGui::InputTextWithHint("##filter", "Filter assets...", search_filter, sizeof(search_filter));

  ImGui::Spacing();

  // Asset table
  const float footer_height = ImGui::GetFrameHeightWithSpacing() + 4.0f;
  if (ImGui::BeginTable("##assets_table", 3,
    ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
    ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp,
    ImVec2(0.0f, -footer_height)))
  {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("##check", ImGuiTableColumnFlags_WidthFixed, 30.0f);
    ImGui::TableSetupColumn("Asset Path", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 90.0f);
    ImGui::TableHeadersRow();

    // Build filter lowercase
    std::string filter_lower;
    if (search_filter[0] != '\0') {
      filter_lower = search_filter;
      std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), ::tolower);
    }

    for (const auto& ae : all_assets) {
      // Apply filter
      if (!filter_lower.empty()) {
        std::string name_lower = ae.relative_path;
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
        if (name_lower.find(filter_lower) == std::string::npos) continue;
      }

      ImGui::TableNextRow();

      // Checkbox column
      ImGui::TableSetColumnIndex(0);
      bool checked = selected_assets.contains(ae.relative_path);
      std::string id = "##" + ae.relative_path;
      if (ImGui::Checkbox(id.c_str(), &checked)) {
        if (checked) selected_assets.insert(ae.relative_path);
        else         selected_assets.erase(ae.relative_path);
        recalculate_total_size();
        save_selection();
      }

      // Path column
      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted(ae.relative_path.c_str());

      // Size column
      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(format_file_size(ae.file_size).c_str());
    }

    ImGui::EndTable();
  }

  // =================================================================
  // Section 3: Export
  // =================================================================
  ImGui::Separator();
  if (ImGui::Button("Export Selected Assets...")) {
    if (selected_assets.empty()) {
      Log::info("BuildSizeAnalyzer: no assets selected for export");
    }
    else {
      auto folder = pick_folder_dialog();
      if (folder.has_value()) {
        do_export(folder.value());
      }
    }
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(%d files, %s)", selected_file_count, format_file_size(total_selected_size).c_str());

  ImGui::End();
}
