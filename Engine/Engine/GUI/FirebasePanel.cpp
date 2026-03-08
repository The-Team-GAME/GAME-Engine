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
}

// ============================================================================
// Panel drawing
// ============================================================================
void FirebasePanel::draw_panel()
{
    ImGui::Begin("Firebase");

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
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 130.f);
        ImGui::InputText("##collection", collection_buf, sizeof(collection_buf));
        ImGui::SameLine();

        if (ImGui::Button("Fetch", ImVec2(60, 0))) {
            std::string col = collection_buf;
            Engine::Firebase::query_collection(col,
                [col](bool success, const std::string& data) {
                    if (success) {
                        parse_collection_response(data);
                        set_status("Fetched " + std::to_string(cached_docs.size()) + " document(s)", true);
                        push_log("QUERY " + col, true,
                            std::to_string(cached_docs.size()) + " document(s)");
                    }
                    else {
                        cached_docs.clear();
                        selected_doc_idx = -1;
                        set_status("Fetch failed: " + data, false);
                        push_log("QUERY " + col, false, data);
                    }
                });
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Enter a Firestore collection name and click Fetch");
            ImGui::Text("to list all documents in that collection.");
            ImGui::EndTooltip();
        }

        // Document list
        ImGui::Text("%zu document(s)", cached_docs.size());
        ImGui::BeginChild("##doclist", ImVec2(0, 150), ImGuiChildFlags_Borders);
        for (int i = 0; i < static_cast<int>(cached_docs.size()); ++i) {
            bool is_selected = (i == selected_doc_idx);
            if (ImGui::Selectable(cached_docs[i].id.c_str(), is_selected)) {
                selected_doc_idx = i;
                // Populate editor
                strncpy_s(doc_id_buf, cached_docs[i].id.c_str(), sizeof(doc_id_buf) - 1);
                strncpy_s(json_edit_buf, cached_docs[i].json.c_str(), sizeof(json_edit_buf) - 1);
            }
        }
        ImGui::EndChild();

        // Delete selected button
        if (selected_doc_idx >= 0 && selected_doc_idx < static_cast<int>(cached_docs.size())) {
            if (ImGui::Button("Delete Selected")) {
                delete_target_id = cached_docs[selected_doc_idx].id;
                confirm_delete_open = true;
                ImGui::OpenPopup("Confirm Delete##collection");
            }
        }

        // Confirmation popup
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
                            // Remove from cached list
                            for (auto it = cached_docs.begin(); it != cached_docs.end(); ++it) {
                                if (it->id == id) { cached_docs.erase(it); break; }
                            }
                            selected_doc_idx = -1;
                            doc_id_buf[0] = '\0';
                            json_edit_buf[0] = '\0';
                            set_status("Deleted " + id, true);
                            push_log("DELETE " + col + "/" + id, true, "OK");
                        }
                        else {
                            set_status("Delete failed: " + data, false);
                            push_log("DELETE " + col + "/" + id, false, data);
                        }
                    });
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
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
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##doc_id", doc_id_buf, sizeof(doc_id_buf));

        ImGui::Spacing();

        // JSON body editor
        ImGui::Text("JSON Body:");
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float editor_height = (avail.y > 400.f) ? 250.f : 180.f;
        ImGui::InputTextMultiline("##json_body", json_edit_buf, sizeof(json_edit_buf),
            ImVec2(-1.f, editor_height), ImGuiInputTextFlags_AllowTabInput);

        ImGui::Spacing();

        // Button row
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
            std::string col = collection_buf;
            std::string id = doc_id_buf;
            std::string body = json_edit_buf;
            Engine::Firebase::set_document(col, id, body,
                [col, id](bool success, const std::string& data) {
                    if (success) {
                        set_status("Saved " + id, true);
                        push_log("SET " + col + "/" + id, true, "OK");
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

        ImGui::SameLine();

        // Format JSON (works even without collection/doc_id)
        if (ImGui::Button("Format JSON")) {
            std::string formatted = pretty_json(json_edit_buf);
            strncpy_s(json_edit_buf, formatted.c_str(), sizeof(json_edit_buf) - 1);
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
                            // Also remove from cached list if present
                            for (auto it = cached_docs.begin(); it != cached_docs.end(); ++it) {
                                if (it->id == id) { cached_docs.erase(it); break; }
                            }
                            selected_doc_idx = -1;
                            set_status("Deleted " + id, true);
                            push_log("DELETE " + col + "/" + id, true, "OK");
                        }
                        else {
                            set_status("Delete failed: " + data, false);
                            push_log("DELETE " + col + "/" + id, false, data);
                        }
                    });
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
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
