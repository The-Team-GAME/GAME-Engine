//-----------------------------------------------------------------------------------
// file:    FirebaseManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of the Firebase Realtime Database manager. Coordinates
//          settings loading, WinHTTP session lifecycle, async CRUD dispatch, and
//          SSE watch management. All HTTP work runs on background threads via
//          std::async; update() polls their futures each frame and drains SSE
//          events, invoking callbacks on the main thread.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "FirebaseManager.h"
#include "FirebaseHTTP.h"
#include "FirebaseListener.h"
#include "FirebaseAuth.h"
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
    static std::string      g_base_url;            // e.g. "https://monkeyengine-2ad5a-default-rtdb.firebaseio.com"

    // Pending async requests
    static std::mutex                    g_mutex;
    static std::vector<PendingRequest>   g_pending;

    // ----------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------

    // Build RTDB REST URL.
    //   rtdb_url("game_data", "player1") → {base_url}/game_data/player1.json
    //   rtdb_url("game_data", "")        → {base_url}/game_data.json
    static std::string rtdb_url(const std::string& collection, const std::string& doc_id = "")
    {
      std::string url = g_base_url + "/" + collection;
      if (!doc_id.empty()) {
        url += "/" + doc_id;
      }
      url += ".json";

      // Append auth token if signed in
      std::string token = Auth::get_id_token();
      if (!token.empty())
        url += "?auth=" + token;

      return url;
    }

    // Enqueue an async HTTP request.
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
      g_base_url = settings.database_url;

      // Trim trailing slash
      if (!g_base_url.empty() && g_base_url.back() == '/') {
        g_base_url.pop_back();
      }

      g_configured = !g_base_url.empty();

      if (!g_configured) {
        Log::warning("[Firebase] database_url not set — cloud features disabled until configured");
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

      // --- Poll CRUD futures ---
      std::vector<PendingRequest> ready;
      {
        std::lock_guard<std::mutex> lock(g_mutex);
        for (auto it = g_pending.begin(); it != g_pending.end(); ) {
          if (it->future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            ready.push_back(std::move(*it));
            it = g_pending.erase(it);
          }
          else {
            ++it;
          }
        }
      }

      for (auto& req : ready) {
        HttpResponse resp = req.future.get();
        if (req.on_complete) {
          req.on_complete(resp.success, resp.body);
        }
      }

      // --- Drain SSE events ---
      Listener::dispatch_events();
    }

    void shutdown()
    {
      if (!g_initialized) return;

      // Stop all watches first
      Listener::stop_all();

      // Wait for outstanding CRUD requests
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
    // RTDB CRUD
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

      std::string url = rtdb_url(collection, document_id);

      enqueue(
        [url]() -> HttpResponse { return HTTP::get(url); },
        [cb = std::move(on_complete)](bool success, const std::string& body) {
          // RTDB returns flat JSON directly — no conversion needed
          cb(success, body);
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

      std::string url = rtdb_url(collection, document_id);

      enqueue(
        [url, json_data]() -> HttpResponse { return HTTP::put(url, json_data); },
        [cb = std::move(on_complete)](bool success, const std::string& body) {
          cb(success, body);
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

      std::string url = rtdb_url(collection, document_id);

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

      std::string url = rtdb_url(collection);

      enqueue(
        [url]() -> HttpResponse { return HTTP::get(url); },
        [cb = std::move(on_complete)](bool success, const std::string& body) {
          // RTDB returns {"doc1":{...},"doc2":{...}} — pass through directly
          cb(success, body);
        }
      );
    }

    void list_collections(Callback on_complete)
    {
      if (!g_configured) {
        if (on_complete) on_complete(false, "Firebase not configured");
        return;
      }

      // GET /.json?shallow=true returns {"collection1":true,"collection2":true}
      std::string url = g_base_url + "/.json?shallow=true";

      // Append auth token if signed in
      std::string token = Auth::get_id_token();
      if (!token.empty())
        url += "&auth=" + token;

      enqueue(
        [url]() -> HttpResponse { return HTTP::get(url); },
        [cb = std::move(on_complete)](bool success, const std::string& body) {
          cb(success, body);
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
          if (method == "PUT")    return HTTP::put(url, body);
          if (method == "PATCH")  return HTTP::patch(url, body);
          if (method == "DELETE") return HTTP::del(url);
          return HttpResponse{ 0, "Unsupported HTTP method: " + method, false };
        },
        std::move(on_complete)
      );
    }

    // ================================================================
    // SSE Watch API — delegates to FirebaseListener
    // ================================================================

    int watch_collection(const std::string& collection, Callback on_change)
    {
      if (!g_configured) return -1;
      std::string url = rtdb_url(collection);
      return Listener::start_watch(url, std::move(on_change));
    }

    int watch_document(const std::string& collection, const std::string& doc_id, Callback on_change)
    {
      if (!g_configured) return -1;
      std::string url = rtdb_url(collection, doc_id);
      return Listener::start_watch(url, std::move(on_change));
    }

    bool unwatch(int watch_id)
    {
      return Listener::stop_watch(watch_id);
    }

    size_t watch_count()
    {
      return Listener::active_count();
    }

    size_t pending_count()
    {
      std::lock_guard<std::mutex> lock(g_mutex);
      return g_pending.size();
    }

  } // namespace Firebase
} // namespace Engine


