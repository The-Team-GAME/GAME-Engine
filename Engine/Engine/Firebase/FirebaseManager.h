//-----------------------------------------------------------------------------------
// file:    FirebaseManager.h
// project: Monkey Stole My GPA
//
// brief:   Public interface for the Firebase Realtime Database integration.
//          Follows the engine namespace-with-free-functions pattern (like
//          AudioSystem). Provides async CRUD operations on RTDB paths
//          and real-time SSE watch/unwatch support. Results are delivered
//          via callbacks on the main thread during update().
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../EngineExport.h"
#include "FirebaseTypes.h"
#include <string>

namespace Engine
{
    namespace Firebase
    {
        // ----------------------------------------------------------------
        // Lifecycle — called from Core::init / Core::run / Core::exit
        // ----------------------------------------------------------------
        ENGINE_API bool init();       // Load settings, open WinHTTP session
        ENGINE_API void update();     // Poll pending futures & SSE events, dispatch callbacks
        ENGINE_API void shutdown();   // Cancel pending work, stop watches, close session

        // ----------------------------------------------------------------
        // Connection status
        // ----------------------------------------------------------------
        ENGINE_API bool is_initialized();
        ENGINE_API bool is_configured();  // True if database_url is set

        // ----------------------------------------------------------------
        // RTDB CRUD  (all async — callback fires on the main thread)
        // ----------------------------------------------------------------

        // GET a single document.
        // Callback receives flat JSON: {"name":"Alice","score":100}
        ENGINE_API void get_document(
            const std::string& collection,
            const std::string& document_id,
            Callback on_complete);

        // CREATE or OVERWRITE a document (PUT).
        // json_data is flat JSON: {"name":"Alice","score":100}
        ENGINE_API void set_document(
            const std::string& collection,
            const std::string& document_id,
            const std::string& json_data,
            Callback on_complete);

        // DELETE a document.
        ENGINE_API void delete_document(
            const std::string& collection,
            const std::string& document_id,
            Callback on_complete);

        // GET all documents in a collection path.
        // Callback receives a JSON object: {"doc1":{...},"doc2":{...}}
        ENGINE_API void query_collection(
            const std::string& collection,
            Callback on_complete);

        // ----------------------------------------------------------------
        // Generic raw HTTP request (for any REST endpoint)
        // ----------------------------------------------------------------
        ENGINE_API void raw_request(
            const std::string& method,   // "GET", "POST", "PUT", "PATCH", "DELETE"
            const std::string& url,
            const std::string& body,
            Callback on_complete);

        // ----------------------------------------------------------------
        // SSE Watch API — real-time listeners
        // ----------------------------------------------------------------
        ENGINE_API int  watch_collection(const std::string& collection, Callback on_change);
        ENGINE_API int  watch_document(const std::string& collection, const std::string& doc_id, Callback on_change);
        ENGINE_API bool unwatch(int watch_id);
        ENGINE_API size_t watch_count();

        // ----------------------------------------------------------------
        // How many CRUD requests are currently in flight
        // ----------------------------------------------------------------
        ENGINE_API size_t pending_count();

    } // namespace Firebase
} // namespace Engine
