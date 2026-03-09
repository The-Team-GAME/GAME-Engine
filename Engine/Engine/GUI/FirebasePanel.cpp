//-----------------------------------------------------------------------------------
// file:    FirebasePanel.cpp
// project: Monkey Stole My GPA
//
// brief:   Implementation of the Firebase database management panel.
//          Sections: Connection Status, Collection Browser, Document Editor,
//          Activity Log.  Persistent panel state is kept in file-scope statics.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "../Precompiled.h"
#include "FirebasePanel.h"
#include "../Firebase/FirebaseManager.h"
#include "../FileIO/Settings.h"
#include "../FileIO/SettingsManager.h"
#include "../Time.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <string>
#include <vector>
#include <deque>
#include <chrono>
#include <ctime>
#include <cstring>
#include <algorithm>
#include <cctype>

// ============================================================================
// Panel-local state
// ============================================================================
namespace
{
  // --- Input buffers ---
  static char collection_buf[256] = "leaderboard";
  static char doc_id_buf[256] = "";
  static char json_edit_buf[8192] = "";
  static char new_field_key[256] = "";
  static char new_field_val[1024] = "";

  // --- Cached collection listing ---
  struct CachedDocument
  {
    std::string id;
    std::string json;       // flat JSON string
  };
  static std::vector<CachedDocument> cached_docs;
  static int selected_doc_idx = -1;

  // --- Activity log ---
  struct LogEntry
  {
    std::string timestamp;
    std::string operation;
    bool        success = false;
    std::string message;
  };
  static std::deque<LogEntry> activity_log;
  static constexpr size_t MAX_LOG_ENTRIES = 200;

  // --- Status bar ---
  static std::string  status_message;
  static ImVec4       status_color = { 1.f, 1.f, 1.f, 1.f };
  static float        status_timer = 0.f;     // seconds remaining
  static constexpr float STATUS_DURATION = 4.f;

  // --- Delete confirmation popup ---
  static bool  confirm_delete_open = false;
  static std::string delete_target_id;

  // --- Collection discovery ---
  static std::vector<std::string> discovered_collections;

  // --- Document filter ---
  static char collection_filter_buf[128] = "";

  // --- Duplicate ---
  static char        duplicate_id_buf[256] = "";
  static std::string duplicate_source_json;

  // --- Panel lifecycle ---
  static bool g_panel_loaded  = false;
  static int  new_doc_counter = 1;

  // --- Field-level editor ---
  struct FieldRow
  {
    char key[256] = {};
    char val[1024] = {};
  };
  static std::vector<FieldRow> field_rows;

  // Helpers
  static std::string now_timestamp()
  {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm buf {};
    localtime_s(&buf, &time);
    char out[16];
    std::snprintf(out, sizeof(out), "%02d:%02d:%02d",
      buf.tm_hour, buf.tm_min, buf.tm_sec);
    return out;
  }

  static void push_log(const std::string& op, bool success, const std::string& msg)
  {
    if (activity_log.size() >= MAX_LOG_ENTRIES) activity_log.pop_front();
    activity_log.push_back({ now_timestamp(), op, success, msg });
  }

  static void set_status(const std::string& msg, bool success)
  {
    status_message = msg;
    status_color = success ? ImVec4(0.2f, 1.f, 0.3f, 1.f)
      : ImVec4(1.f, 0.3f, 0.2f, 1.f);
    status_timer = STATUS_DURATION;
  }

  static std::string pretty_json(const char* raw)
  {
    rapidjson::Document doc;
    doc.Parse(raw);
    if (doc.HasParseError()) return raw;

    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
    writer.SetIndent(' ', 2);
    doc.Accept(writer);
    return buf.GetString();
  }

  // Parse the flat JSON array from query_collection into CachedDocument list
  static void parse_collection_response(const std::string& json_array)
  {
    cached_docs.clear();
    selected_doc_idx = -1;

    rapidjson::Document doc;
    doc.Parse(json_array.c_str());
    if (doc.HasParseError() || !doc.IsArray()) return;

    for (rapidjson::SizeType i = 0; i < doc.Size(); ++i) {
      const auto& obj = doc[i];
      if (!obj.IsObject()) continue;

      CachedDocument cd;

      // Extract _id
      if (obj.HasMember("_id") && obj["_id"].IsString()) {
        cd.id = obj["_id"].GetString();
      }

      // Serialize the whole object (minus _id) as flat JSON for display
      rapidjson::Document copy;
      copy.SetObject();
      auto& alloc = copy.GetAllocator();
      for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
        if (std::string(it->name.GetString()) == "_id") continue;
        copy.AddMember(
          rapidjson::Value(it->name, alloc),
          rapidjson::Value(it->value, alloc),
          alloc);
      }
      rapidjson::StringBuffer buf;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
      writer.SetIndent(' ', 2);
      copy.Accept(writer);
      cd.json = buf.GetString();

      cached_docs.push_back(std::move(cd));
    }
  }

  // ----------------------------------------------------------------
  // Persist pinned collections via firebase_settings.json
  // ----------------------------------------------------------------
  static void save_pinned_collections()
  {
    auto& settings = Engine::FileIO::SettingsManager<Engine::FileIO::FirebaseSettings>::get_mutable();
    settings.pinned_collections = discovered_collections;
    Engine::FileIO::SettingsManager<Engine::FileIO::FirebaseSettings>::save();
  }

  static void load_pinned_collections()
  {
    const auto& settings = Engine::FileIO::SettingsManager<Engine::FileIO::FirebaseSettings>::get();
    discovered_collections = settings.pinned_collections;
  }

  // Add col to pinned list and persist (no-op if already present)
  static void pin_collection(const std::string& col)
  {
    if (col.empty()) return;
    for (const auto& s : discovered_collections) if (s == col) return;
    discovered_collections.push_back(col);
    save_pinned_collections();
  }

  // ----------------------------------------------------------------
  // Write a string as the most specific JSON type that fits
  // ----------------------------------------------------------------
  static void write_inferred_value(const char* str,
    rapidjson::Value& out,
    rapidjson::Document::AllocatorType& alloc)
  {
    std::string s(str);
    if (s == "true") { out.SetBool(true);  return; }
    if (s == "false") { out.SetBool(false); return; }
    if (s == "null" || s.empty()) { out.SetNull();      return; }
    try { size_t p; long long iv = std::stoll(s, &p); if (p == s.size()) { out.SetInt64(iv);  return; } }
    catch (...) {}
    try { size_t p; double   dv = std::stod(s, &p); if (p == s.size()) { out.SetDouble(dv); return; } }
    catch (...) {}
    out.SetString(s.c_str(), static_cast<rapidjson::SizeType>(s.size()), alloc);
  }

  // Serialize field_rows -> json_edit_buf (keeps the raw editor in sync)
  static void sync_fields_to_json()
  {
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();
    for (const auto& row : field_rows)
    {
      if (row.key[0] == '\0') continue;
      rapidjson::Value val;
      write_inferred_value(row.val, val, alloc);
      doc.AddMember(rapidjson::Value(row.key, alloc), val, alloc);
    }
    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
    writer.SetIndent(' ', 2);
    doc.Accept(writer);
    strncpy_s(json_edit_buf, buf.GetString(), sizeof(json_edit_buf) - 1);
  }

  // Parse json_edit_buf -> field_rows (called whenever a document is loaded)
  static void sync_json_to_fields()
  {
    field_rows.clear();
    rapidjson::Document doc;
    doc.Parse(json_edit_buf);
    if (doc.HasParseError() || !doc.IsObject()) return;
    for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
    {
      FieldRow row{};
      strncpy_s(row.key, it->name.GetString(), sizeof(row.key) - 1);
      if (it->value.IsString()) {
        strncpy_s(row.val, it->value.GetString(), sizeof(row.val) - 1);
      }
      else if (it->value.IsBool()) {
        strncpy_s(row.val, it->value.GetBool() ? "true" : "false", sizeof(row.val) - 1);
      }
      else if (it->value.IsNull()) {
        strncpy_s(row.val, "null", sizeof(row.val) - 1);
      }
      else {
        rapidjson::StringBuffer sbuf;
        rapidjson::Writer<rapidjson::StringBuffer> w(sbuf);
        it->value.Accept(w);
        strncpy_s(row.val, sbuf.GetString(), sizeof(row.val) - 1);
      }
      field_rows.push_back(row);
    }
  }
}

// ============================================================================
// Panel drawing
// ============================================================================
void FirebasePanel::draw_panel()
{
  ImGui::Begin("Firebase");

  if (!g_panel_loaded) { load_pinned_collections(); g_panel_loaded = true; }

  const bool initialized = Engine::Firebase::is_initialized();
  const bool configured = Engine::Firebase::is_configured();

  // Tick status timer
  if (status_timer > 0.f) {
    status_timer -= Engine::Time::get_real_dt();
    if (status_timer <= 0.f) {
      status_message.clear();
      status_timer = 0.f;
    }
  }

  // ========================================================================
  // Connection Status
  // ========================================================================
  if (ImGui::CollapsingHeader("Connection Status", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // Status indicators
    ImGui::Text("Initialized:");
    ImGui::SameLine();
    ImGui::TextColored(initialized ? ImVec4(0.2f, 1.f, 0.3f, 1.f) : ImVec4(1.f, 0.3f, 0.2f, 1.f),
      initialized ? "Yes" : "No");

    ImGui::Text("Configured: ");
    ImGui::SameLine();
    ImGui::TextColored(configured ? ImVec4(0.2f, 1.f, 0.3f, 1.f) : ImVec4(1.f, 0.3f, 0.2f, 1.f),
      configured ? "Yes" : "No");

    // Show project ID
    const auto& settings = Engine::FileIO::SettingsManager<Engine::FileIO::FirebaseSettings>::get();
    ImGui::Text("Project ID: ");
    ImGui::SameLine();
    if (settings.project_id.empty()) {
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f), "(not set)");
    }
    else {
      ImGui::TextWrapped("%s", settings.project_id.c_str());
    }

    // Pending requests
    size_t pending = Engine::Firebase::pending_count();
    if (pending > 0) {
      ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f), "%zu request(s) in flight", pending);
    }

    ImGui::Spacing();

    if (!configured) {
      ImGui::TextColored(ImVec4(1.f, 0.8f, 0.f, 1.f),
        "Configure project_id and api_key in:");
      ImGui::BulletText("Assets/settings/firebase_settings.json");
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f),
        "Restart the engine after editing.");
    }
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Disable all interactive sections when not configured
  if (!configured) ImGui::BeginDisabled();

  // ========================================================================
  // Collection Browser
  // ========================================================================
  if (ImGui::CollapsingHeader("Collection Browser", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // ---- Pinned collections list ----
    ImGui::Text("Collections:");
    ImGui::BeginChild("##collectionlist", ImVec2(0, 80), ImGuiChildFlags_Borders);
    if (discovered_collections.empty()) {
      ImGui::TextDisabled("  (type a name below and click Pin)");
    }
    else {
      int remove_idx = -1;
      for (int ci = 0; ci < static_cast<int>(discovered_collections.size()); ++ci)
      {
        const auto& col_name = discovered_collections[ci];
        bool is_active = (col_name == collection_buf);
        if (ImGui::Selectable(col_name.c_str(), is_active, ImGuiSelectableFlags_None, ImVec2(ImGui::GetContentRegionAvail().x - 22.f, 0)))
        {
          strncpy_s(collection_buf, col_name.c_str(), sizeof(collection_buf) - 1);
          Engine::Firebase::query_collection(col_name,
            [col_name](bool success, const std::string& data) {
              if (success) {
                parse_collection_response(data);
                set_status("Fetched " + std::to_string(cached_docs.size()) + " document(s)", true);
                push_log("QUERY " + col_name, true,
                  std::to_string(cached_docs.size()) + " document(s)");
              }
              else {
                cached_docs.clear();
                selected_doc_idx = -1;
                set_status("Fetch failed: " + data, false);
                push_log("QUERY " + col_name, false, data);
              }
            });
        }
        ImGui::SameLine();
        char rbtn[32]; std::snprintf(rbtn, sizeof(rbtn), "x##rmc%d", ci);
        if (ImGui::SmallButton(rbtn)) remove_idx = ci;
      }
      if (remove_idx >= 0) {
        discovered_collections.erase(discovered_collections.begin() + remove_idx);
        save_pinned_collections();
      }
    }
    ImGui::EndChild();

    // ---- Collection name input + Add (fetches and pins on success) ----
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 48.f);
    bool add_enter = ImGui::InputText("##collection", collection_buf, sizeof(collection_buf),
      ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::Button("Add", ImVec2(-1.f, 0)) || add_enter) {
      std::string col = collection_buf;
      Engine::Firebase::query_collection(col,
        [col](bool success, const std::string& data) {
          if (success) {
            parse_collection_response(data);
            pin_collection(col);
            set_status("Added \"" + col + "\" — " + std::to_string(cached_docs.size()) + " document(s)", true);
            push_log("ADD " + col, true, std::to_string(cached_docs.size()) + " document(s)");
          }
          else {
            cached_docs.clear();
            selected_doc_idx = -1;
            set_status("Failed to add \"" + col + "\": " + data, false);
            push_log("ADD " + col, false, data);
          }
        });
    }

    // ---- Filter ----
    ImGui::Text("Filter:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputText("##docfilter", collection_filter_buf, sizeof(collection_filter_buf));

    // ---- Document list (filtered) ----
    {
      std::string lower_filter = collection_filter_buf;
      for (auto& c : lower_filter) c = (char)std::tolower((unsigned char)c);

      int visible_count = 0;
      for (const auto& d : cached_docs) {
        if (lower_filter.empty()) { ++visible_count; continue; }
        std::string lower_id = d.id;
        for (auto& c : lower_id) c = (char)std::tolower((unsigned char)c);
        if (lower_id.find(lower_filter) != std::string::npos) ++visible_count;
      }

      if (!lower_filter.empty())
        ImGui::Text("%d / %zu document(s)", visible_count, cached_docs.size());
      else
        ImGui::Text("%zu document(s)", cached_docs.size());

      ImGui::BeginChild("##doclist", ImVec2(0, 150), ImGuiChildFlags_Borders);
      for (int i = 0; i < static_cast<int>(cached_docs.size()); ++i)
      {
        if (!lower_filter.empty()) {
          std::string lower_id = cached_docs[i].id;
          for (auto& ch : lower_id) ch = (char)std::tolower((unsigned char)ch);
          if (lower_id.find(lower_filter) == std::string::npos) continue;
        }
        bool is_selected = (i == selected_doc_idx);
        if (ImGui::Selectable(cached_docs[i].id.c_str(), is_selected)) {
          selected_doc_idx = i;
          strncpy_s(doc_id_buf, cached_docs[i].id.c_str(), sizeof(doc_id_buf) - 1);
          strncpy_s(json_edit_buf, cached_docs[i].json.c_str(), sizeof(json_edit_buf) - 1);
          sync_json_to_fields();
        }
      }
      ImGui::EndChild();
    }

    // ---- Delete Selected | Duplicate ----
    if (selected_doc_idx >= 0 && selected_doc_idx < static_cast<int>(cached_docs.size()))
    {
      if (ImGui::Button("Delete Selected")) {
        delete_target_id = cached_docs[selected_doc_idx].id;
        confirm_delete_open = true;
        ImGui::OpenPopup("Confirm Delete##collection");
      }
      ImGui::SameLine();
      if (ImGui::Button("Duplicate...")) {
        duplicate_source_json = cached_docs[selected_doc_idx].json;
        std::string new_name = cached_docs[selected_doc_idx].id + "_copy";
        strncpy_s(duplicate_id_buf, new_name.c_str(), sizeof(duplicate_id_buf) - 1);
        ImGui::OpenPopup("Duplicate Document");
      }
    }

    // Confirmation popup — delete
    if (ImGui::BeginPopupModal("Confirm Delete##collection", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Delete document \"%s\"?", delete_target_id.c_str());
      ImGui::Text("This cannot be undone.");
      ImGui::Spacing();

      if (ImGui::Button("Delete", ImVec2(120, 0))) {
        std::string col = collection_buf;
        std::string id = delete_target_id;
        Engine::Firebase::delete_document(col, id,
          [col, id](bool success, const std::string& data) {
            if (success) {
              for (auto it = cached_docs.begin(); it != cached_docs.end(); ++it)
                if (it->id == id) { cached_docs.erase(it); break; }
              selected_doc_idx = -1;
              doc_id_buf[0] = '\0';
              json_edit_buf[0] = '\0';
              field_rows.clear();
              set_status("Deleted " + id, true);
              push_log("DELETE " + col + "/" + id, true, "OK");
              Engine::Firebase::query_collection(col,
                [col](bool ok, const std::string& d2) {
                  if (ok) parse_collection_response(d2);
                });
            }
            else {
              set_status("Delete failed: " + data, false);
              push_log("DELETE " + col + "/" + id, false, data);
            }
          });
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0)))
        ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
    }

    // Confirmation popup — duplicate
    if (ImGui::BeginPopupModal("Duplicate Document", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::Text("New document ID:");
      ImGui::SetNextItemWidth(300.f);
      ImGui::InputText("##dup_id", duplicate_id_buf, sizeof(duplicate_id_buf));
      ImGui::Spacing();

      bool can_dup = (duplicate_id_buf[0] != '\0');
      if (!can_dup) ImGui::BeginDisabled();
      if (ImGui::Button("Duplicate", ImVec2(120, 0)))
      {
        std::string col = collection_buf;
        std::string new_id = duplicate_id_buf;
        std::string body = duplicate_source_json;
        Engine::Firebase::set_document(col, new_id, body,
          [col, new_id](bool success, const std::string& data) {
            if (success) {
              set_status("Duplicated to " + new_id, true);
              push_log("DUPLICATE -> " + col + "/" + new_id, true, "OK");
              Engine::Firebase::query_collection(col,
                [col](bool ok, const std::string& d2) {
                  if (ok) parse_collection_response(d2);
                });
            }
            else {
              set_status("Duplicate failed: " + data, false);
              push_log("DUPLICATE -> " + col + "/" + new_id, false, data);
            }
          });
        ImGui::CloseCurrentPopup();
      }
      if (!can_dup) ImGui::EndDisabled();
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0)))
        ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
    }
  }

  ImGui::Spacing();

  // ========================================================================
  // Document Editor
  // ========================================================================
  if (ImGui::CollapsingHeader("Document Editor", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // Document ID
    ImGui::Text("Document ID:");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 135.f);
    ImGui::InputText("##doc_id", doc_id_buf, sizeof(doc_id_buf));
    ImGui::SameLine();
    if (ImGui::Button("New Document", ImVec2(-1.f, 0))) {
      selected_doc_idx = -1;
      std::string new_id = "new_doc_" + std::to_string(new_doc_counter++);
      strncpy_s(doc_id_buf, new_id.c_str(), sizeof(doc_id_buf) - 1);
      json_edit_buf[0] = '{';
      json_edit_buf[1] = '}';
      json_edit_buf[2] = '\0';
      field_rows.clear();
    }

    ImGui::Spacing();

    // ---- Field-level editor (A) ----
    ImGui::Text("Fields:");
    if (ImGui::BeginTable("##fields", 3,
      ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV |
      ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch, 0.35f);
      ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.57f);
      ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 28.f);
      ImGui::TableHeadersRow();

      int erase_idx = -1;
      for (int fi = 0; fi < static_cast<int>(field_rows.size()); ++fi)
      {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::SetNextItemWidth(-1.f);
        char kid[32]; std::snprintf(kid, sizeof(kid), "##fk%d", fi);
        if (ImGui::InputText(kid, field_rows[fi].key, sizeof(field_rows[fi].key)))
          sync_fields_to_json();

        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.f);
        char vid[32]; std::snprintf(vid, sizeof(vid), "##fv%d", fi);
        if (ImGui::InputText(vid, field_rows[fi].val, sizeof(field_rows[fi].val)))
          sync_fields_to_json();

        ImGui::TableSetColumnIndex(2);
        char rid[32]; std::snprintf(rid, sizeof(rid), "x##fr%d", fi);
        if (ImGui::SmallButton(rid)) erase_idx = fi;
      }

      // Add-row
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::SetNextItemWidth(-1.f);
      ImGui::InputText("##nfk", new_field_key, sizeof(new_field_key));
      ImGui::TableSetColumnIndex(1);
      ImGui::SetNextItemWidth(-1.f);
      ImGui::InputText("##nfv", new_field_val, sizeof(new_field_val));
      ImGui::TableSetColumnIndex(2);
      if (ImGui::SmallButton("+##addfield") && new_field_key[0] != '\0') {
        FieldRow nr{};
        strncpy_s(nr.key, new_field_key, sizeof(nr.key) - 1);
        strncpy_s(nr.val, new_field_val, sizeof(nr.val) - 1);
        field_rows.push_back(nr);
        new_field_key[0] = '\0';
        new_field_val[0] = '\0';
        sync_fields_to_json();
      }

      ImGui::EndTable();

      if (erase_idx >= 0) {
        field_rows.erase(field_rows.begin() + erase_idx);
        sync_fields_to_json();
      }
    }

    ImGui::Spacing();

    // ---- Action buttons ----
    bool has_doc_id = (doc_id_buf[0] != '\0');
    bool has_collection = (collection_buf[0] != '\0');

    if (!has_collection || !has_doc_id) ImGui::BeginDisabled();

    if (ImGui::Button("Get")) {
      std::string col = collection_buf;
      std::string id = doc_id_buf;
      Engine::Firebase::get_document(col, id,
        [col, id](bool success, const std::string& data) {
          if (success) {
            std::string formatted = pretty_json(data.c_str());
            strncpy_s(json_edit_buf, formatted.c_str(), sizeof(json_edit_buf) - 1);
            sync_json_to_fields();
            set_status("Loaded " + id, true);
            push_log("GET " + col + "/" + id, true, "OK");
          }
          else {
            set_status("Get failed: " + data, false);
            push_log("GET " + col + "/" + id, false, data);
          }
        });
    }

    ImGui::SameLine();

    if (ImGui::Button("Save")) {
      sync_fields_to_json();   // flush field editor -> json_edit_buf
      std::string col = collection_buf;
      std::string id = doc_id_buf;
      std::string body = json_edit_buf;
      Engine::Firebase::set_document(col, id, body,
        [col, id](bool success, const std::string& data) {
          if (success) {
            set_status("Saved " + id, true);
            push_log("SET " + col + "/" + id, true, "OK");
            Engine::Firebase::query_collection(col,
              [col](bool ok, const std::string& d2) {
                if (ok) parse_collection_response(d2);
              });
          }
          else {
            set_status("Save failed: " + data, false);
            push_log("SET " + col + "/" + id, false, data);
          }
        });
    }

    ImGui::SameLine();

    if (ImGui::Button("Delete")) {
      delete_target_id = doc_id_buf;
      ImGui::OpenPopup("Confirm Delete##editor");
    }

    if (!has_collection || !has_doc_id) ImGui::EndDisabled();

    ImGui::Spacing();

    // ---- Raw JSON (advanced) ----
    if (ImGui::TreeNode("Raw JSON (advanced)"))
    {
      ImVec2 avail = ImGui::GetContentRegionAvail();
      float editor_height = (avail.y > 400.f) ? 200.f : 150.f;
      if (ImGui::InputTextMultiline("##json_body", json_edit_buf, sizeof(json_edit_buf),
        ImVec2(-1.f, editor_height), ImGuiInputTextFlags_AllowTabInput))
      {
        sync_json_to_fields();
      }
      if (ImGui::Button("Format JSON")) {
        std::string formatted = pretty_json(json_edit_buf);
        strncpy_s(json_edit_buf, formatted.c_str(), sizeof(json_edit_buf) - 1);
        sync_json_to_fields();
      }
      ImGui::TreePop();
    }

    // Delete confirmation popup (editor version)
    if (ImGui::BeginPopupModal("Confirm Delete##editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Delete document \"%s\"?", delete_target_id.c_str());
      ImGui::Text("This cannot be undone.");
      ImGui::Spacing();

      if (ImGui::Button("Delete", ImVec2(120, 0))) {
        std::string col = collection_buf;
        std::string id = delete_target_id;
        Engine::Firebase::delete_document(col, id,
          [col, id](bool success, const std::string& data) {
            if (success) {
              doc_id_buf[0] = '\0';
              json_edit_buf[0] = '\0';
              field_rows.clear();
              for (auto it = cached_docs.begin(); it != cached_docs.end(); ++it)
                if (it->id == id) { cached_docs.erase(it); break; }
              selected_doc_idx = -1;
              set_status("Deleted " + id, true);
              push_log("DELETE " + col + "/" + id, true, "OK");
              Engine::Firebase::query_collection(col,
                [col](bool ok, const std::string& d2) {
                  if (ok) parse_collection_response(d2);
                });
            }
            else {
              set_status("Delete failed: " + data, false);
              push_log("DELETE " + col + "/" + id, false, data);
            }
          });
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0)))
        ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
    }
  }

  if (!configured) ImGui::EndDisabled();

  ImGui::Spacing();

  // ========================================================================
  // Activity Log
  // ========================================================================
  if (ImGui::CollapsingHeader("Activity Log"))
  {
    if (ImGui::Button("Clear Log")) {
      activity_log.clear();
    }
    ImGui::SameLine();
    ImGui::Text("%zu entries", activity_log.size());

    ImGui::BeginChild("##activity_log", ImVec2(0, 160), ImGuiChildFlags_Borders);
    for (const auto& entry : activity_log) {
      ImVec4 col = entry.success
        ? ImVec4(0.2f, 1.f, 0.3f, 1.f)
        : ImVec4(1.f, 0.3f, 0.2f, 1.f);

      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f), "[%s]", entry.timestamp.c_str());
      ImGui::SameLine();
      ImGui::TextColored(col, "%s", entry.operation.c_str());
      ImGui::SameLine();
      ImGui::TextColored(col, "— %s", entry.message.c_str());
    }

    // Auto-scroll to bottom
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20.f) {
      ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
  }

  // ========================================================================
  // Status bar
  // ========================================================================
  if (!status_message.empty()) {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(status_color, "%s", status_message.c_str());
  }

  ImGui::End();
}
