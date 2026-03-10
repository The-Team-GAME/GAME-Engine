//-----------------------------------------------------------------------------------
// file:    FirebasePanel.cpp
// project: Monkey Stole My GPA
//
// brief:   Firebase Firestore CRUD panel.
//          Sections: Connection Status, Collection Browser, Document Editor,
//          Activity Log.
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

  // Parse the flat JSON array from query_collection into CachedDocument list.
  static void parse_collection_response(const std::string& json_array)
  {
    cached_docs.clear();
    selected_doc_idx = -1;

    rapidjson::Document doc;
    doc.Parse(json_array.c_str());
    if (doc.HasParseError() || !doc.IsArray()) return;

    for (rapidjson::SizeType i = 0; i < doc.Size(); ++i)
    {
      const auto& obj = doc[i];
      if (!obj.IsObject()) continue;

      CachedDocument cd;
      if (obj.HasMember("_id") && obj["_id"].IsString())
        cd.id = obj["_id"].GetString();

      // Serialize the object (minus _id) as flat JSON
      rapidjson::Document copy;
      copy.SetObject();
      auto& alloc = copy.GetAllocator();
      for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
      {
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

  // Fetch the current collection and handle all outcomes.
  static void fetch_collection()
  {
    const auto& settings =
      Engine::FileIO::SettingsManager<Engine::FileIO::FirebaseSettings>::get();
    std::string col = settings.collection;
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
    ImGui::Text("Project ID: ");
    ImGui::SameLine();
    if (settings.project_id.empty())
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f), "(not set)");
    else
      ImGui::TextWrapped("%s", settings.project_id.c_str());

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
        "Configure project_id and api_key in:");
      ImGui::BulletText("Assets/settings/firebase_settings.json");
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f),
        "Restart the engine after editing.");
    }
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (!configured) ImGui::BeginDisabled();

  // ========================================================================
  // Collection Browser
  // ========================================================================

  // Auto-fetch on first frame when configured
  const auto& fb_settings =
    Engine::FileIO::SettingsManager<Engine::FileIO::FirebaseSettings>::get();
  static bool auto_fetched = false;
  if (configured && !auto_fetched && !fb_settings.collection.empty())
  {
    auto_fetched = true;
    fetch_collection();
  }

  if (ImGui::CollapsingHeader("Collection Browser", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // ---- Collection name (read-only from settings) + Refresh ----
    ImGui::Text("Collection:");
    ImGui::SameLine();
    if (fb_settings.collection.empty())
      ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f), "(not set — edit firebase_settings.json)");
    else
      ImGui::TextWrapped("%s", fb_settings.collection.c_str());

    bool can_fetch = (!fb_settings.collection.empty() && pending == 0);
    if (!can_fetch) ImGui::BeginDisabled();
    if (ImGui::Button("Refresh", ImVec2(-1.f, 0)))
      fetch_collection();
    if (!can_fetch) ImGui::EndDisabled();

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
      ImGui::TextDisabled("Waiting for initial fetch...");
    }
    else if (cached_docs.empty())
    {
      ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f),
        "No documents found.");
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f),
        "The collection may not exist or is empty.");
    }
    else
    {
      for (int i = 0; i < static_cast<int>(cached_docs.size()); ++i)
      {
        bool is_selected = (i == selected_doc_idx);
        std::string label = cached_docs[i].id;
        if (is_empty_document_json(cached_docs[i].json))
          label += " (empty)";

        if (ImGui::Selectable(label.c_str(), is_selected))
        {
          selected_doc_idx = i;
          strncpy_s(doc_id_buf, cached_docs[i].id.c_str(), sizeof(doc_id_buf) - 1);
          strncpy_s(json_edit_buf, cached_docs[i].json.c_str(), sizeof(json_edit_buf) - 1);
          sync_json_to_fields();
        }
      }
    }
    ImGui::EndChild();

    // ---- New + Delete buttons ----
    if (ImGui::Button("+ New"))
    {
      std::string default_name = "new_doc_" + std::to_string(new_doc_counter++);
      strncpy_s(new_doc_name_buf, default_name.c_str(), sizeof(new_doc_name_buf) - 1);
      ImGui::OpenPopup("New Document");
    }

    if (selected_doc_idx >= 0 && selected_doc_idx < static_cast<int>(cached_docs.size()))
    {
      ImGui::SameLine();
      bool can_delete = (!fb_settings.collection.empty() && pending == 0);
      if (!can_delete) ImGui::BeginDisabled();
      if (ImGui::Button("Delete Selected"))
      {
        delete_target_id = cached_docs[selected_doc_idx].id;
        ImGui::OpenPopup("Confirm Delete");
      }
      if (!can_delete) ImGui::EndDisabled();
    }

    // ---- Delete confirmation popup ----
    if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::Text("Delete document \"%s\"?", delete_target_id.c_str());
      ImGui::Text("This cannot be undone.");
      ImGui::Spacing();

      if (ImGui::Button("Delete", ImVec2(120, 0)))
      {
        std::string col = fb_settings.collection;
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
      ImGui::Text("Document name:");
      ImGui::SetNextItemWidth(300.f);
      bool enter = ImGui::InputText("##newdocname", new_doc_name_buf, sizeof(new_doc_name_buf),
        ImGuiInputTextFlags_EnterReturnsTrue);

      ImGui::Spacing();

      bool can_create = (new_doc_name_buf[0] != '\0' && !fb_settings.collection.empty());
      if (!can_create) ImGui::BeginDisabled();
      if (ImGui::Button("Create", ImVec2(120, 0)) || (enter && can_create))
      {
        selected_doc_idx = -1;
        strncpy_s(doc_id_buf, new_doc_name_buf, sizeof(doc_id_buf) - 1);
        json_edit_buf[0] = '{'; json_edit_buf[1] = '}'; json_edit_buf[2] = '\0';
        field_rows.clear();

        // Push the new (empty) document to Firestore immediately
        std::string col  = fb_settings.collection;
        std::string id   = new_doc_name_buf;
        std::string body = "{}";
        Engine::Firebase::set_document(col, id, body,
          [col, id](bool success, const std::string& data) {
            if (success) {
              set_status("Created " + id, true);
              push_log("CREATE " + col + "/" + id, true, "OK");
              // Refresh the collection list so the new doc appears
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
              set_status("Create failed: " + data, false);
              push_log("CREATE " + col + "/" + id, false, data);
            }
          });

        push_log("NEW", true, std::string(new_doc_name_buf));
        ImGui::CloseCurrentPopup();
      }
      if (!can_create) ImGui::EndDisabled();
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
    // ---- Document ID + Pull/Push ----
    bool has_doc_id = (doc_id_buf[0] != '\0');
    bool has_collection = !fb_settings.collection.empty();
    bool can_act = has_doc_id && has_collection && pending == 0;

    ImGui::Text("Document ID:");
    ImGui::SameLine();
    if (doc_id_buf[0] != '\0')
      ImGui::TextWrapped("%s", doc_id_buf);
    else
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f), "(none selected)");

    if (!can_act) ImGui::BeginDisabled();

    // Pull — fetch from Firestore into editor
    if (ImGui::Button("Pull"))
    {
      std::string col = fb_settings.collection;
      std::string id = doc_id_buf;
      Engine::Firebase::get_document(col, id,
        [col, id](bool success, const std::string& data) {
          if (success) {
            std::string formatted = pretty_json(data.c_str());
            strncpy_s(json_edit_buf, formatted.c_str(), sizeof(json_edit_buf) - 1);
            sync_json_to_fields();
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

    // Push — write editor contents to Firestore
    if (ImGui::Button("Push"))
    {
      sync_fields_to_json();
      std::string col = fb_settings.collection;
      std::string id = doc_id_buf;
      std::string body = json_edit_buf;
      Engine::Firebase::set_document(col, id, body,
        [col, id](bool success, const std::string& data) {
          if (success) {
            set_status("Pushed " + id, true);
            push_log("PUSH " + col + "/" + id, true, "OK");
            // Refresh the collection list
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
            set_status("Push failed: " + data, false);
            push_log("PUSH " + col + "/" + id, false, data);
          }
        });
    }

    if (!can_act) ImGui::EndDisabled();

    ImGui::Spacing();

    // ---- Field-level editor ----
    ImGui::Text("Fields:");
    if (has_doc_id && field_rows.empty())
    {
      ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f),
        "This document has no fields.");
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f),
        "Add a field below or push {} to keep it empty.");
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
      if (ImGui::SmallButton("+##addfield") && new_field_key[0] != '\0')
      {
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
