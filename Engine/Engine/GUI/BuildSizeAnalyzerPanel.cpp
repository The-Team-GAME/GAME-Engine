//-----------------------------------------------------------------------------------
// file:    BuildSizeAnalyzerPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
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
  std::string                      export_status_msg;

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
  const float footer_height = ImGui::GetFrameHeightWithSpacing() * 2.0f + 8.0f;
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
  // Export
  // =================================================================
  ImGui::Separator();
  ImGui::Spacing();

  if (ImGui::Button("Export Selected Assets...")) {
    if (selected_assets.empty()) {
      export_status_msg = "Nothing selected.";
    }
    else {
      auto folder = pick_folder_dialog();
      if (folder.has_value()) {
        do_export_snapshot(selected_assets, Engine::Resources::resource_path, folder.value(), "Assets");
        export_status_msg = "Done! Exported to: " + folder.value();
      }
    }
  }

  ImGui::SameLine();
  ImGui::TextDisabled("(%d files, %s)", selected_file_count,
                      format_file_size(total_selected_size).c_str());

  if (!export_status_msg.empty()) {
    ImGui::TextDisabled("%s", export_status_msg.c_str());
  }

  ImGui::End();
}


