//-----------------------------------------------------------------------------------
// file:    FirebasePanel.cpp
// project: Monkey Stole My GPA
//
// brief:   Firebase Realtime Database CRUD panel with Live Update.
//          Sections: Connection Status, Collection Browser, Document Editor,
//          Live Update, Activity Log.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "../Precompiled.h"
#include "FirebasePanel.h"
#include "../Firebase/FirebaseManager.h"
#include "../FileIO/Settings.h"
#include "../FileIO/SettingsManager.h"
#include "../Time.h"
#include "../Logger.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <cstring>

// ============================================================================
// Panel-local state
// ============================================================================
namespace
{
  // --- Input buffers ---
  static char doc_id_buf[256] = "";
  static char json_edit_buf[8192] = "";
  static char new_field_key[256] = "";
  static char new_field_val[1024] = "";

  // --- Cached collection listing ---
  struct CachedDocument
  {
    std::string id;
    std::string json;
  };
  static std::vector<CachedDocument> cached_docs;
  static int selected_doc_idx = -1;

  // --- Fetch state ---
  static bool has_fetched_once = false;  // distinguishes "never fetched" from "fetched, 0 results"
  static int  new_doc_counter = 1;

  // --- Activity log ---
  struct PanelLogEntry
  {
    std::string timestamp;
    std::string operation;
    bool        success = false;
    bool        warning = false;
    std::string message;
  };
  static std::deque<PanelLogEntry> activity_log;
  static constexpr size_t MAX_LOG_ENTRIES = 200;

  // --- Status bar ---
  static std::string status_message;
  static ImVec4      status_color = { 1.f, 1.f, 1.f, 1.f };
  static float       status_timer = 0.f;
  static constexpr float STATUS_DURATION = 4.f;

  // --- Delete confirmation ---
  static std::string delete_target_id;

  // --- New document popup ---
  static char new_doc_name_buf[256] = "";

  // --- Field-level editor ---
  struct FieldRow
  {
    char key[256] = {};
    char val[1024] = {};
  };
  static std::vector<FieldRow> field_rows;

  // --- Live Update ---
  static bool  live_update_enabled = true;
  static int   live_watch_id = -1;
  static int   live_event_count = 0;
  static std::string live_last_event_time;

  // --- Collections ---
  static std::vector<std::string> cached_collections;
  static std::string selected_collection;
  static bool has_fetched_collections = false;
  static char new_collection_buf[256] = "";
  static std::string delete_collection_target;
  static bool fields_dirty = false;

  // --- Create wizard (shared by New Collection step-2 and New Document) ---
  static int  new_col_step = 0;    // 0 = col ID, 1 = doc+field
  static char new_doc_field_name[128] = "";
  static int  new_doc_field_type = 0;  // 0 = String, 1 = Number
  static char new_doc_field_value[256] = "";

  // ====================================================================
  // Helpers
  // ====================================================================

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

  static void push_log(const std::string& op, bool success, const std::string& msg,
    bool warning = false)
  {
    if (activity_log.size() >= MAX_LOG_ENTRIES) activity_log.pop_front();
    activity_log.push_back({ now_timestamp(), op, success, warning, msg });

    if (warning)
      Log::warning("[Firebase] {} — {}", op, msg);
    else if (success)
      Log::info("[Firebase] {} — {}", op, msg);
    else
      Log::error("[Firebase] {} — {}", op, msg);
  }

  static void set_status(const std::string& msg, bool success, bool warning = false)
  {
    status_message = msg;
    if (warning)
      status_color = ImVec4(1.f, 0.85f, 0.f, 1.f);   // yellow
    else if (success)
      status_color = ImVec4(0.2f, 1.f, 0.3f, 1.f);   // green
    else
      status_color = ImVec4(1.f, 0.3f, 0.2f, 1.f);   // red
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

  static bool is_empty_document_json(const std::string& json)
  {
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    return !doc.HasParseError() && doc.IsObject() && doc.ObjectEmpty();
  }

  // Parse the RTDB collection response into CachedDocument list.
  // RTDB returns an object keyed by doc ID: {"doc1":{...},"doc2":{...}}
  // or "null" if the path doesn't exist.
  static void parse_collection_response(const std::string& json_obj)
  {
    cached_docs.clear();
    selected_doc_idx = -1;

    rapidjson::Document doc;
    doc.Parse(json_obj.c_str());
    if (doc.HasParseError() || doc.IsNull()) return;
    if (!doc.IsObject()) return;

    for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
    {
      CachedDocument cd;
      cd.id = it->name.GetString();

      // Serialize the value as pretty JSON
      rapidjson::StringBuffer buf;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
      writer.SetIndent(' ', 2);
      it->value.Accept(writer);
      cd.json = buf.GetString();

      cached_docs.push_back(std::move(cd));
    }
  }

  // Write a string as the most specific JSON type that fits.
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
    try { size_t p; double   dv = std::stod(s, &p);  if (p == s.size()) { out.SetDouble(dv); return; } }
    catch (...) {}
    out.SetString(s.c_str(), static_cast<rapidjson::SizeType>(s.size()), alloc);
  }

  // Serialize field_rows -> json_edit_buf
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

  // Parse json_edit_buf -> field_rows
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
      if (it->value.IsString())
        strncpy_s(row.val, it->value.GetString(), sizeof(row.val) - 1);
      else if (it->value.IsBool())
        strncpy_s(row.val, it->value.GetBool() ? "true" : "false", sizeof(row.val) - 1);
      else if (it->value.IsNull())
        strncpy_s(row.val, "null", sizeof(row.val) - 1);
      else {
        rapidjson::StringBuffer sbuf;
        rapidjson::Writer<rapidjson::StringBuffer> w(sbuf);
        it->value.Accept(w);
        strncpy_s(row.val, sbuf.GetString(), sizeof(row.val) - 1);
      }
      field_rows.push_back(row);
    }
  }

  // Fetch the list of top-level collection names from RTDB root.
  static void fetch_all_collections()
  {
    Engine::Firebase::list_collections(
      [](bool success, const std::string& data) {
        has_fetched_collections = true;
        if (!success) {
          set_status("Failed to list collections: " + data, false);
          push_log("LIST COLLECTIONS", false, data);
          return;
        }
        cached_collections.clear();
        rapidjson::Document doc;
        doc.Parse(data.c_str());
        if (!doc.HasParseError() && doc.IsObject()) {
          for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
            cached_collections.push_back(it->name.GetString());
          std::sort(cached_collections.begin(), cached_collections.end());
        }
        push_log("LIST COLLECTIONS", true,
          std::to_string(cached_collections.size()) + " collection(s)");
      });
  }

  // Fetch documents within the currently selected collection.
  static void fetch_collection()
  {
    std::string col = selected_collection;
    if (col.empty()) return;

    Engine::Firebase::query_collection(col,
      [col](bool success, const std::string& data) {
        has_fetched_once = true;

        if (!success) {
          cached_docs.clear();
          selected_doc_idx = -1;
          set_status("Fetch failed: " + data, false);
          push_log("QUERY " + col, false, data);
          return;
        }

        parse_collection_response(data);

        if (cached_docs.empty()) {
          set_status("No documents found — collection may not exist or is empty", true, true);
          push_log("QUERY " + col, true,
            "0 documents — collection may not exist or is empty", true);
        }
        else {
          set_status("Fetched " + std::to_string(cached_docs.size()) + " document(s) from \"" + col + "\"", true);
          push_log("QUERY " + col, true,
            std::to_string(cached_docs.size()) + " document(s)");

          // Auto-pull the selected document if one is active
          std::string selected_id(doc_id_buf);
          if (!selected_id.empty()) {
            // Re-select the same doc in the refreshed list
            for (int i = 0; i < static_cast<int>(cached_docs.size()); ++i) {
              if (cached_docs[i].id == selected_id) {
                selected_doc_idx = i;
                break;
              }
            }
            Engine::Firebase::get_document(col, selected_id,
              [col, selected_id](bool ok, const std::string& doc_data) {
                if (ok) {
                  std::string formatted = pretty_json(doc_data.c_str());
                  strncpy_s(json_edit_buf, formatted.c_str(), sizeof(json_edit_buf) - 1);
                  sync_json_to_fields();
                  push_log("PULL " + col + "/" + selected_id, true, "OK");
                }
              });
          }
        }
      });
  }
}

// ============================================================================
// Panel drawing
// ============================================================================
void FirebasePanel::draw_panel()
{
  ImGui::Begin("Firebase");

  const bool initialized = Engine::Firebase::is_initialized();
  const bool configured = Engine::Firebase::is_configured();
  const size_t pending = Engine::Firebase::pending_count();

  // Tick status timer
  if (status_timer > 0.f)
  {
    status_timer -= Engine::Time::get_real_dt();
    if (status_timer <= 0.f) { status_message.clear(); status_timer = 0.f; }
  }

  // ========================================================================
  // Connection Status
  // ========================================================================
  if (ImGui::CollapsingHeader("Connection Status", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Text("Initialized:");
    ImGui::SameLine();
    ImGui::TextColored(initialized ? ImVec4(0.2f, 1.f, 0.3f, 1.f)
      : ImVec4(1.f, 0.3f, 0.2f, 1.f),
      initialized ? "Yes" : "No");

    ImGui::Text("Configured: ");
    ImGui::SameLine();
    ImGui::TextColored(configured ? ImVec4(0.2f, 1.f, 0.3f, 1.f)
      : ImVec4(1.f, 0.3f, 0.2f, 1.f),
      configured ? "Yes" : "No");

    const auto& settings =
      Engine::FileIO::SettingsManager<Engine::FileIO::FirebaseSettings>::get();
    ImGui::Text("Database URL:");
    ImGui::SameLine();
    if (settings.database_url.empty())
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f), "(not set)");
    else
      ImGui::TextWrapped("%s", settings.database_url.c_str());

    if (pending > 0)
    {
      // Animated dots
      static float dot_timer = 0.f;
      dot_timer += Engine::Time::get_real_dt();
      int dots = (static_cast<int>(dot_timer * 3.f) % 3) + 1;
      char dot_str[4] = "...";
      dot_str[dots] = '\0';
      ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f),
        "%zu request(s) in flight%s", pending, dot_str);
    }

    ImGui::Spacing();

    if (!configured)
    {
      ImGui::TextColored(ImVec4(1.f, 0.8f, 0.f, 1.f),
        "Configure database_url in:");
      ImGui::BulletText("Assets/settings/firebase_settings.json");
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f),
        "Restart the engine after editing.");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ---- Live Update toggle ----
    if (!configured) ImGui::BeginDisabled();
    bool lu_toggled = ImGui::Checkbox("Live Update", &live_update_enabled);
    if (lu_toggled)
    {
      if (live_update_enabled)
      {
        std::string col = selected_collection;
        if (!col.empty())
        {
          live_event_count = 0;
          live_last_event_time.clear();
          live_watch_id = Engine::Firebase::watch_collection(col,
            [col](bool /*success*/, const std::string& event_json) {
              live_event_count++;
              live_last_event_time = now_timestamp();
              push_log("SSE", true, event_json.substr(0, 120));
              Engine::Firebase::query_collection(col,
                [col](bool ok, const std::string& data) {
                  if (ok) {
                    has_fetched_once = true;
                    std::string prev_id(doc_id_buf);
                    parse_collection_response(data);
                    if (!prev_id.empty()) {
                      for (int i = 0; i < static_cast<int>(cached_docs.size()); ++i) {
                        if (cached_docs[i].id == prev_id) {
                          selected_doc_idx = i;
                          strncpy_s(json_edit_buf, cached_docs[i].json.c_str(), sizeof(json_edit_buf) - 1);
                          sync_json_to_fields();
                          break;
                        }
                      }
                    }
                  }
                });
            });
          if (live_watch_id >= 0) { set_status("Live Update started on \"" + col + "\"", true); push_log("WATCH", true, "Started on " + col); }
          else { live_update_enabled = false; set_status("Failed to start Live Update", false); push_log("WATCH", false, "Failed"); }
        }
        else { live_update_enabled = false; }
      }
      else
      {
        if (live_watch_id >= 0) { Engine::Firebase::unwatch(live_watch_id); push_log("UNWATCH", true, "Stopped watch " + std::to_string(live_watch_id)); live_watch_id = -1; }
        set_status("Live Update stopped", true);
      }
    }
    ImGui::SameLine();
    if (live_update_enabled && live_watch_id >= 0)
      ImGui::TextColored(ImVec4(0.2f, 1.f, 0.3f, 1.f), "connected");
    else
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f), "off");
    if (live_event_count > 0)
    {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f),
        "%d event(s), last: %s", live_event_count, live_last_event_time.c_str());
    }
    if (!configured) ImGui::EndDisabled();
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (!configured) ImGui::BeginDisabled();

  // ========================================================================
  // Collections
  // ========================================================================

  // Auto-fetch collection list on first configured frame
  static bool auto_fetched = false;
  if (configured && !auto_fetched)
  {
    auto_fetched = true;
    fetch_all_collections();
  }

  if (ImGui::CollapsingHeader("Collections", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // ---- Buttons above list: Refresh | + New | Delete ----
    bool can_list = (pending == 0);
    if (!can_list) ImGui::BeginDisabled();
    if (ImGui::Button("Refresh##col_refresh"))
      fetch_all_collections();
    if (!can_list) ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("+ New##col_new"))
    {
      new_col_step = 0;
      new_collection_buf[0] = '\0';
      ImGui::OpenPopup("New Collection");
    }

    bool has_col_selected = !selected_collection.empty();
    ImGui::SameLine();
    if (!has_col_selected || !can_list) ImGui::BeginDisabled();
    if (ImGui::Button("Delete##col_del"))
    {
      delete_collection_target = selected_collection;
      ImGui::OpenPopup("Confirm Delete Collection");
    }
    if (!has_col_selected || !can_list) ImGui::EndDisabled();
    if (has_col_selected)
    {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.f, 1.f), "%s", selected_collection.c_str());
    }

    ImGui::Spacing();

    // ---- Collection list ----
    ImGui::BeginChild("##collist", ImVec2(0, 100), ImGuiChildFlags_Borders);
    if (!has_fetched_collections)
    {
      ImGui::TextDisabled("Loading...");
    }
    else if (cached_collections.empty())
    {
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f),
        "No collections found. Use '+ New' to create one.");
    }
    else
    {
      for (const auto& col_name : cached_collections)
      {
        bool is_selected = (col_name == selected_collection);
        if (ImGui::Selectable(col_name.c_str(), is_selected))
        {
          if (!is_selected)
          {
            selected_collection = col_name;
            cached_docs.clear();
            selected_doc_idx = -1;
            has_fetched_once = false;
            doc_id_buf[0] = '\0';
            json_edit_buf[0] = '\0';
            field_rows.clear();
            fields_dirty = false;
            fetch_collection();
          }
        }
      }
    }
    ImGui::EndChild();

    // ---- Delete Collection popup ----
    if (ImGui::BeginPopupModal("Confirm Delete Collection", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::Text("Delete collection \"%s\"?", delete_collection_target.c_str());
      ImGui::Text("ALL documents inside will be permanently removed.");
      ImGui::Spacing();
      if (ImGui::Button("Delete", ImVec2(120, 0)))
      {
        std::string col = delete_collection_target;
        Engine::Firebase::delete_document(col, "",
          [col](bool success, const std::string& data) {
            if (success) {
              if (selected_collection == col) {
                selected_collection.clear();
                cached_docs.clear();
                selected_doc_idx = -1;
                has_fetched_once = false;
                doc_id_buf[0] = '\0';
                json_edit_buf[0] = '\0';
                field_rows.clear();
              }
              cached_collections.erase(
                std::remove(cached_collections.begin(), cached_collections.end(), col),
                cached_collections.end());
              set_status("Deleted collection \"" + col + "\"", true);
              push_log("DELETE COLLECTION", true, col);
            }
            else {
              set_status("Delete collection failed: " + data, false);
              push_log("DELETE COLLECTION", false, data);
            }
          });
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0)))
        ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
    }

    // ---- New Collection popup (2-step wizard) ----
    if (ImGui::BeginPopupModal("New Collection", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      if (new_col_step == 0)
      {
        // ---- Step 1: Collection ID ----
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f), "Step 1 of 2");
        ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Collection ID (no spaces):");
        ImGui::SetNextItemWidth(300.f);
        bool enter = ImGui::InputText("##newcolname", new_collection_buf, sizeof(new_collection_buf),
          ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();
        bool can_next = (new_collection_buf[0] != '\0');
        if (!can_next) ImGui::BeginDisabled();
        if (ImGui::Button("Next", ImVec2(120, 0)) || (enter && can_next))
        {
          strncpy_s(new_doc_name_buf, "document_1", sizeof(new_doc_name_buf) - 1);
          new_doc_field_name[0] = '\0';
          new_doc_field_type = 0;
          new_doc_field_value[0] = '\0';
          new_col_step = 1;
        }
        if (!can_next) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel##col1", ImVec2(120, 0)))
          ImGui::CloseCurrentPopup();
      }
      else
      {
        // ---- Step 2: First Document + First Field ----
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f), "Step 2 of 2");
        ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Collection:"); ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.f, 1.f), "%s", new_collection_buf);
        ImGui::Spacing();
        ImGui::Text("Document ID:");
        ImGui::SetNextItemWidth(300.f);
        ImGui::InputText("##ncdocid", new_doc_name_buf, sizeof(new_doc_name_buf));
        ImGui::Spacing(); ImGui::Separator();
        ImGui::Text("First Field"); ImGui::Spacing();
        ImGui::SetNextItemWidth(180.f);
        ImGui::InputText("Name##ncff", new_doc_field_name, sizeof(new_doc_field_name));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110.f);
        const char* type_items[] = { "String", "Number" };
        ImGui::Combo("Type##ncff", &new_doc_field_type, type_items, 2);
        ImGui::SetNextItemWidth(300.f);
        ImGui::InputText("Value##ncff", new_doc_field_value, sizeof(new_doc_field_value));
        ImGui::Spacing();
        bool can_create = (new_doc_name_buf[0] != '\0'
          && new_doc_field_name[0] != '\0'
          && new_doc_field_value[0] != '\0');
        if (ImGui::Button("Back", ImVec2(80, 0)))
          new_col_step = 0;
        ImGui::SameLine();
        if (!can_create) ImGui::BeginDisabled();
        if (ImGui::Button("Create", ImVec2(120, 0)))
        {
          std::string col = new_collection_buf;
          std::string id = new_doc_name_buf;
          std::string fname = new_doc_field_name;
          std::string fval = new_doc_field_value;
          std::string body = (new_doc_field_type == 1)
            ? "{\"" + fname + "\":" + fval + "}"
            : "{\"" + fname + "\":\"" + fval + "\"}";
          if (std::find(cached_collections.begin(), cached_collections.end(), col)
            == cached_collections.end())
            cached_collections.push_back(col);
          selected_collection = col;
          cached_docs.clear();
          selected_doc_idx = -1;
          has_fetched_once = true;
          CachedDocument nd; nd.id = id; nd.json = body;
          cached_docs.push_back(nd);
          selected_doc_idx = 0;
          strncpy_s(doc_id_buf, id.c_str(), sizeof(doc_id_buf) - 1);
          strncpy_s(json_edit_buf, body.c_str(), sizeof(json_edit_buf) - 1);
          field_rows.clear(); sync_json_to_fields(); fields_dirty = false;
          Engine::Firebase::set_document(col, id, body,
            [col, id](bool success, const std::string& data) {
              if (success) { set_status("Created " + col + "/" + id, true);   push_log("CREATE " + col + "/" + id, true, "OK"); }
              else { set_status("Create failed: " + data, false); push_log("CREATE " + col + "/" + id, false, data); }
            });
          push_log("NEW COLLECTION+DOC", true, col + "/" + id);
          ImGui::CloseCurrentPopup();
        }
        if (!can_create) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel##col2", ImVec2(120, 0)))
          ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }

  ImGui::Spacing();

  // ========================================================================
  // Document Browser
  // ========================================================================

  if (ImGui::CollapsingHeader("Document Browser", ImGuiTreeNodeFlags_DefaultOpen))
  {
    if (selected_collection.empty())
    {
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f),
        "Select a collection above to browse documents.");
    }
    else
    {
      ImGui::Text("Collection:");
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.f, 1.f), "%s", selected_collection.c_str());

      // ---- Buttons above list: Refresh | + New | Delete ----
      bool can_fetch = (pending == 0);
      if (!can_fetch) ImGui::BeginDisabled();
      if (ImGui::Button("Refresh##docs"))
        fetch_collection();
      if (!can_fetch) ImGui::EndDisabled();

      ImGui::SameLine();
      if (ImGui::Button("+ New##doc_new"))
      {
        std::string default_name = "new_doc_" + std::to_string(new_doc_counter++);
        strncpy_s(new_doc_name_buf, default_name.c_str(), sizeof(new_doc_name_buf) - 1);
        new_doc_field_name[0] = '\0';
        new_doc_field_type = 0;
        new_doc_field_value[0] = '\0';
        ImGui::OpenPopup("New Document");
      }

      bool has_doc_sel = (selected_doc_idx >= 0 && selected_doc_idx < static_cast<int>(cached_docs.size()));
      ImGui::SameLine();
      if (!has_doc_sel || !can_fetch) ImGui::BeginDisabled();
      if (ImGui::Button("Delete##doc_del"))
      {
        delete_target_id = cached_docs[selected_doc_idx].id;
        ImGui::OpenPopup("Confirm Delete");
      }
      if (!has_doc_sel || !can_fetch) ImGui::EndDisabled();

      ImGui::Spacing();

      // ---- Document list ----
      ImGui::Text("%zu document(s)", cached_docs.size());

      ImGui::BeginChild("##doclist", ImVec2(0, 160), ImGuiChildFlags_Borders);
      if (pending > 0 && !has_fetched_once)
      {
        ImGui::TextDisabled("Loading...");
      }
      else if (!has_fetched_once)
      {
        ImGui::TextDisabled("Press Refresh to load documents.");
      }
      else if (cached_docs.empty())
      {
        ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f), "No documents found.");
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f),
          "The collection may be empty or not yet created.");
      }
      else
      {
        for (int i = 0; i < static_cast<int>(cached_docs.size()); ++i)
        {
          bool is_selected = (i == selected_doc_idx);
          const std::string& label = cached_docs[i].id;
          if (ImGui::Selectable(label.c_str(), is_selected))
          {
            selected_doc_idx = i;
            fields_dirty = false;
            strncpy_s(doc_id_buf, cached_docs[i].id.c_str(), sizeof(doc_id_buf) - 1);
            strncpy_s(json_edit_buf, cached_docs[i].json.c_str(), sizeof(json_edit_buf) - 1);
            sync_json_to_fields();
          }
        }
      }
      ImGui::EndChild();

      // ---- Delete confirmation popup ----
      if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
      {
        ImGui::Text("Delete document \"%s\"?", delete_target_id.c_str());
        ImGui::Text("This cannot be undone.");
        ImGui::Spacing();
        if (ImGui::Button("Delete", ImVec2(120, 0)))
        {
          std::string col = selected_collection;
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
                    if (ok) {
                      has_fetched_once = true;
                      parse_collection_response(d2);
                      push_log("REFRESH " + col, true,
                        std::to_string(cached_docs.size()) + " document(s)");
                    }
                    else {
                      push_log("REFRESH " + col, false, d2);
                    }
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

      // ---- New Document popup ----
      if (ImGui::BeginPopupModal("New Document", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
      {
        ImGui::Text("Collection:"); ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.f, 1.f), "%s", selected_collection.c_str());
        ImGui::Spacing();
        ImGui::Text("Document ID:");
        ImGui::SetNextItemWidth(300.f);
        ImGui::InputText("##newdocid", new_doc_name_buf, sizeof(new_doc_name_buf));
        ImGui::Spacing(); ImGui::Separator();
        ImGui::Text("First Field"); ImGui::Spacing();
        ImGui::SetNextItemWidth(180.f);
        ImGui::InputText("Name##ndff", new_doc_field_name, sizeof(new_doc_field_name));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110.f);
        const char* type_items[] = { "String", "Number" };
        ImGui::Combo("Type##ndff", &new_doc_field_type, type_items, 2);
        ImGui::SetNextItemWidth(300.f);
        ImGui::InputText("Value##ndff", new_doc_field_value, sizeof(new_doc_field_value));
        ImGui::Spacing();
        bool can_create = (new_doc_name_buf[0] != '\0'
          && new_doc_field_name[0] != '\0'
          && new_doc_field_value[0] != '\0');
        if (!can_create) ImGui::BeginDisabled();
        if (ImGui::Button("Create", ImVec2(120, 0)))
        {
          std::string col = selected_collection;
          std::string id = new_doc_name_buf;
          std::string fname = new_doc_field_name;
          std::string fval = new_doc_field_value;
          std::string body = (new_doc_field_type == 1)
            ? "{\"" + fname + "\":" + fval + "}"
            : "{\"" + fname + "\":\"" + fval + "\"}";
          CachedDocument nd; nd.id = id; nd.json = body;
          cached_docs.push_back(nd);
          has_fetched_once = true;
          selected_doc_idx = static_cast<int>(cached_docs.size()) - 1;
          strncpy_s(doc_id_buf, id.c_str(), sizeof(doc_id_buf) - 1);
          strncpy_s(json_edit_buf, body.c_str(), sizeof(json_edit_buf) - 1);
          field_rows.clear(); sync_json_to_fields(); fields_dirty = false;
          Engine::Firebase::set_document(col, id, body,
            [col, id](bool success, const std::string& data) {
              if (success) { set_status("Created " + id, true);            push_log("CREATE " + col + "/" + id, true, "OK"); }
              else { set_status("Create failed: " + data, false);  push_log("CREATE " + col + "/" + id, false, data); }
            });
          push_log("NEW DOC", true, col + "/" + id);
          ImGui::CloseCurrentPopup();
        }
        if (!can_create) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
          ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
      }
    }
  }

  ImGui::Spacing();

  // ========================================================================
  // Document Editor
  // ========================================================================
  if (ImGui::CollapsingHeader("Document Editor", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // ---- Document ID + Pull/Push ----
    bool has_doc_id = (doc_id_buf[0] != '\0');
    bool has_collection = !selected_collection.empty();
    bool can_act = has_doc_id && has_collection && pending == 0;

    ImGui::Text("Document ID:");
    ImGui::SameLine();
    if (doc_id_buf[0] != '\0')
      ImGui::TextWrapped("%s", doc_id_buf);
    else
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f), "(none selected)");

    // Pull / Push — only available when Live Update is disabled
    if (!live_update_enabled)
    {
      if (!can_act) ImGui::BeginDisabled();

      if (ImGui::Button("Pull from Server"))
      {
        std::string col = selected_collection;
        std::string id = doc_id_buf;
        Engine::Firebase::get_document(col, id,
          [col, id](bool success, const std::string& data) {
            if (success) {
              std::string formatted = pretty_json(data.c_str());
              strncpy_s(json_edit_buf, formatted.c_str(), sizeof(json_edit_buf) - 1);
              sync_json_to_fields();
              fields_dirty = false;
              set_status("Pulled " + id, true);
              push_log("PULL " + col + "/" + id, true, "OK");
            }
            else {
              set_status("Pull failed: " + data, false);
              push_log("PULL " + col + "/" + id, false, data);
            }
          });
      }

      ImGui::SameLine();

      if (ImGui::Button("Push to Server"))
      {
        sync_fields_to_json();
        std::string col = selected_collection;
        std::string id = doc_id_buf;
        std::string body = json_edit_buf;
        Engine::Firebase::set_document(col, id, body,
          [col, id](bool success, const std::string& data) {
            if (success) {
              fields_dirty = false;
              set_status("Pushed " + id, true);
              push_log("PUSH " + col + "/" + id, true, "OK");
              Engine::Firebase::query_collection(col,
                [col](bool ok, const std::string& d2) {
                  if (ok) { has_fetched_once = true; parse_collection_response(d2); }
                  else { push_log("REFRESH " + col, false, d2); }
                });
            }
            else {
              set_status("Push failed: " + data, false);
              push_log("PUSH " + col + "/" + id, false, data);
            }
          });
      }

      if (!can_act) ImGui::EndDisabled();
    }

    ImGui::Spacing();

    // ---- Field-level editor ----
    ImGui::Text("Fields:");

    // Always keep one trailing empty placeholder row so the user can type
    // a new field directly — no separate Add button needed.
    {
      bool trailing_empty = !field_rows.empty()
        && field_rows.back().key[0] == '\0'
        && field_rows.back().val[0] == '\0';
      if (!trailing_empty)
        field_rows.emplace_back();
    }

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
        {
          sync_fields_to_json(); fields_dirty = true;
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.f);
        char vid[32]; std::snprintf(vid, sizeof(vid), "##fv%d", fi);
        if (ImGui::InputText(vid, field_rows[fi].val, sizeof(field_rows[fi].val)))
        {
          sync_fields_to_json(); fields_dirty = true;
        }

        ImGui::TableSetColumnIndex(2);
        // Hide the delete button on the trailing empty placeholder row
        bool is_trailing_empty = (fi == static_cast<int>(field_rows.size()) - 1
          && field_rows[fi].key[0] == '\0'
          && field_rows[fi].val[0] == '\0');
        if (!is_trailing_empty)
        {
          char rid[32]; std::snprintf(rid, sizeof(rid), "x##fr%d", fi);
          if (ImGui::SmallButton(rid)) erase_idx = fi;
        }
      }

      ImGui::EndTable();

      if (erase_idx >= 0) {
        field_rows.erase(field_rows.begin() + erase_idx);
        sync_fields_to_json();
        if (doc_id_buf[0] != '\0' && !selected_collection.empty())
        {
          std::string col = selected_collection;
          std::string id = doc_id_buf;
          std::string body = json_edit_buf;
          Engine::Firebase::set_document(col, id, body,
            [col, id](bool success, const std::string& data) {
              if (success) {
                set_status("Field deleted — " + id + " saved", true);
                push_log("DEL FIELD " + col + "/" + id, true, "OK");
                Engine::Firebase::query_collection(col,
                  [col](bool ok, const std::string& d2) {
                    if (ok) { has_fetched_once = true; parse_collection_response(d2); }
                  });
              }
              else {
                set_status("Delete field failed: " + data, false);
                push_log("DEL FIELD", false, data);
              }
            });
        }
      }
    }

    // ---- Update button (shown only when a field was modified and not yet saved) ----
    if (fields_dirty)
    {
      ImGui::Spacing();
      if (can_act)
      {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.15f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.75f, 0.20f, 1.f));
        if (ImGui::Button("Update##field_update", ImVec2(-1.f, 0)))
        {
          sync_fields_to_json();
          std::string col = selected_collection;
          std::string id = doc_id_buf;
          std::string body = json_edit_buf;
          Engine::Firebase::set_document(col, id, body,
            [col, id](bool success, const std::string& data) {
              if (success) {
                fields_dirty = false;
                set_status("Updated " + id, true);
                push_log("UPDATE " + col + "/" + id, true, "OK");
                Engine::Firebase::query_collection(col,
                  [col](bool ok, const std::string& d2) {
                    if (ok) { has_fetched_once = true; parse_collection_response(d2); }
                    else { push_log("REFRESH " + col, false, d2); }
                  });
              }
              else {
                set_status("Update failed: " + data, false);
                push_log("UPDATE " + col + "/" + id, false, data);
              }
            });
        }
        ImGui::PopStyleColor(2);
      }
      else
      {
        ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f), "Unsaved changes");
      }
    }

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
        fields_dirty = true;
      }
      if (ImGui::Button("Format JSON"))
      {
        std::string formatted = pretty_json(json_edit_buf);
        strncpy_s(json_edit_buf, formatted.c_str(), sizeof(json_edit_buf) - 1);
        sync_json_to_fields();
      }
      ImGui::TreePop();
    }
  }

  if (!configured) ImGui::EndDisabled();

  ImGui::Spacing();

  // ========================================================================
  // Activity Log
  // ========================================================================
  if (ImGui::CollapsingHeader("Activity Log"))
  {
    if (ImGui::Button("Clear Log"))
      activity_log.clear();
    ImGui::SameLine();
    ImGui::Text("%zu entries", activity_log.size());

    ImGui::BeginChild("##activity_log", ImVec2(0, 160), ImGuiChildFlags_Borders);
    for (const auto& entry : activity_log)
    {
      ImVec4 col;
      if (entry.warning)
        col = ImVec4(1.f, 0.85f, 0.f, 1.f);    // yellow
      else if (entry.success)
        col = ImVec4(0.2f, 1.f, 0.3f, 1.f);    // green
      else
        col = ImVec4(1.f, 0.3f, 0.2f, 1.f);    // red

      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f), "[%s]", entry.timestamp.c_str());
      ImGui::SameLine();
      ImGui::TextColored(col, "%s", entry.operation.c_str());
      ImGui::SameLine();
      ImGui::TextColored(col, "— %s", entry.message.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20.f)
      ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
  }

  // ========================================================================
  // Status bar
  // ========================================================================
  if (!status_message.empty())
  {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(status_color, "%s", status_message.c_str());
  }

  ImGui::End();
}
