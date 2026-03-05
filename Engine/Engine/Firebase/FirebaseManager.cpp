//-----------------------------------------------------------------------------------
// file:    FirebaseManager.cpp
// project: Monkey Stole My GPA
//
// brief:   Implementation of the Firebase Cloud Firestore manager.  Coordinates
//          settings loading, WinHTTP session lifecycle, and async request dispatch.
//          All HTTP work runs on background threads via std::async; update() polls
//          their futures each frame and invokes callbacks on the main thread.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "FirebaseManager.h"
#include "FirebaseHTTP.h"
#include "../Logger.h"
#include "../FileIO/Settings.h"
#include "../FileIO/SettingsManager.h"

#include <vector>
#include <mutex>
#include <future>
#include <chrono>

namespace Engine
{
    namespace Firebase
    {
        // ================================================================
        // Internal state
        // ================================================================
        static bool             g_initialized = false;
        static bool             g_configured = false;
        static std::string      g_base_url;            // e.g. "https://firestore.googleapis.com"
        static std::string      g_project_id;
        static std::string      g_api_key;

        // Pending async requests — guarded by mutex because std::async worker
        // threads could be modifying futures concurrently with the main thread.
        static std::mutex                    g_mutex;
        static std::vector<PendingRequest>   g_pending;

        // ----------------------------------------------------------------
        // Helpers
        // ----------------------------------------------------------------

        // Build Firestore REST URL for a document or collection.
        //   document_path("scores", "player1")  →
        //     https://firestore.googleapis.com/v1/projects/{id}/databases/(default)/documents/scores/player1?key={key}
        //   document_path("scores", "")  →
        //     .../documents/scores?key={key}
        static std::string document_url(const std::string& collection, const std::string& doc_id = "")
        {
            std::string url = g_base_url
                + "/v1/projects/" + g_project_id
                + "/databases/(default)/documents/"
                + collection;

            if (!doc_id.empty()) {
                url += "/" + doc_id;
            }

            if (!g_api_key.empty()) {
                url += "?key=" + g_api_key;
            }

            return url;
        }

        // Enqueue an async HTTP request.  The lambda `work` runs on a
        // background thread; when it finishes, update() picks up the result.
        static void enqueue(std::function<HttpResponse()> work, Callback on_complete)
        {
            PendingRequest req;
            req.future = std::async(std::launch::async, std::move(work));
            req.on_complete = std::move(on_complete);

            std::lock_guard<std::mutex> lock(g_mutex);
            g_pending.push_back(std::move(req));
        }

        // ================================================================
        // Lifecycle
        // ================================================================

        bool init()
        {
            if (g_initialized) return true;

            // Load firebase settings
            if (!Engine::FileIO::SettingsManager<Engine::FileIO::FirebaseSettings>::load(
                "Assets/settings/firebase_settings.json")) {
                Log::warning("[Firebase] Failed to load firebase_settings.json, cloud features disabled");
            }

            const auto& settings = Engine::FileIO::SettingsManager<Engine::FileIO::FirebaseSettings>::get();
            g_project_id = settings.project_id;
            g_api_key = settings.api_key;
            g_base_url = settings.database_url.empty()
                ? "https://firestore.googleapis.com"
                : settings.database_url;

            // Trim trailing slash
            if (!g_base_url.empty() && g_base_url.back() == '/') {
                g_base_url.pop_back();
            }

            g_configured = !g_project_id.empty() && !g_api_key.empty();

            if (!g_configured) {
                Log::warning("[Firebase] project_id or api_key not set — cloud features disabled until configured");
            }

            // Open WinHTTP session
            if (!HTTP::open_session("MonkeyEngine/1.0")) {
                Log::error("[Firebase] Failed to open WinHTTP session");
                return false;
            }

            g_initialized = true;
            Log::info("[Firebase] Initialized (configured: {})", g_configured ? "yes" : "no");
            return true;
        }

        void update()
        {
            if (!g_initialized) return;

            // Snapshot the pending list so we don't hold the lock while invoking callbacks
            std::vector<PendingRequest> ready;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                for (auto it = g_pending.begin(); it != g_pending.end(); ) {
                    // Check if the future is ready (non-blocking)
                    if (it->future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                        ready.push_back(std::move(*it));
                        it = g_pending.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }

            // Dispatch callbacks on the main thread
            for (auto& req : ready) {
                HttpResponse resp = req.future.get();
                if (req.on_complete) {
                    req.on_complete(resp.success, resp.body);
                }
            }
        }

        void shutdown()
        {
            if (!g_initialized) return;

            // Wait for outstanding requests to finish (with a timeout)
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                for (auto& req : g_pending) {
                    if (req.future.valid()) {
                        req.future.wait_for(std::chrono::seconds(2));
                    }
                }
                g_pending.clear();
            }

            HTTP::close_session();

            g_initialized = false;
            g_configured = false;
            Log::info("[Firebase] Shutdown complete");
        }

        bool is_initialized() { return g_initialized; }
        bool is_configured() { return g_configured; }

        // ================================================================
        // Firestore CRUD
        // ================================================================

        void get_document(
            const std::string& collection,
            const std::string& document_id,
            Callback on_complete)
        {
            if (!g_configured) {
                if (on_complete) on_complete(false, "Firebase not configured");
                return;
            }

            std::string url = document_url(collection, document_id);

            enqueue(
                [url]() -> HttpResponse { return HTTP::get(url); },
                [cb = std::move(on_complete)](bool success, const std::string& body) {
                    if (success) {
                        cb(true, from_firestore_document(body));
                    }
                    else {
                        cb(false, body);
                    }
                }
            );
        }

        void set_document(
            const std::string& collection,
            const std::string& document_id,
            const std::string& json_data,
            Callback on_complete)
        {
            if (!g_configured) {
                if (on_complete) on_complete(false, "Firebase not configured");
                return;
            }

            std::string url = document_url(collection, document_id);
            std::string firestore_body = to_firestore_document(json_data);

            enqueue(
                [url, firestore_body]() -> HttpResponse { return HTTP::patch(url, firestore_body); },
                [cb = std::move(on_complete)](bool success, const std::string& body) {
                    if (success) {
                        cb(true, from_firestore_document(body));
                    }
                    else {
                        cb(false, body);
                    }
                }
            );
        }

        void delete_document(
            const std::string& collection,
            const std::string& document_id,
            Callback on_complete)
        {
            if (!g_configured) {
                if (on_complete) on_complete(false, "Firebase not configured");
                return;
            }

            std::string url = document_url(collection, document_id);

            enqueue(
                [url]() -> HttpResponse { return HTTP::del(url); },
                [cb = std::move(on_complete)](bool success, const std::string& body) {
                    cb(success, body);
                }
            );
        }

        void query_collection(
            const std::string& collection,
            Callback on_complete)
        {
            if (!g_configured) {
                if (on_complete) on_complete(false, "Firebase not configured");
                return;
            }

            std::string url = document_url(collection);

            enqueue(
                [url]() -> HttpResponse { return HTTP::get(url); },
                [cb = std::move(on_complete)](bool success, const std::string& body) {
                    if (success) {
                        cb(true, from_firestore_collection(body));
                    }
                    else {
                        cb(false, body);
                    }
                }
            );
        }

        // ================================================================
        // Generic raw request
        // ================================================================

        void raw_request(
            const std::string& method,
            const std::string& url,
            const std::string& body,
            Callback on_complete)
        {
            if (!g_initialized) {
                if (on_complete) on_complete(false, "Firebase not initialized");
                return;
            }

            enqueue(
                [method, url, body]() -> HttpResponse {
                    if (method == "GET")    return HTTP::get(url);
                    if (method == "POST")   return HTTP::post(url, body);
                    if (method == "PATCH")  return HTTP::patch(url, body);
                    if (method == "DELETE") return HTTP::del(url);
                    return HttpResponse{ 0, "Unsupported HTTP method: " + method, false };
                },
                std::move(on_complete)
            );
        }

        size_t pending_count()
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            return g_pending.size();
        }

    } // namespace Firebase
} // namespace Engine
