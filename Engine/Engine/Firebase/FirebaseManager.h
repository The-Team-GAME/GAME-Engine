//-----------------------------------------------------------------------------------
// file:    FirebaseManager.h
// project: Monkey Stole My GPA
//
// brief:   Public interface for the Firebase Cloud Firestore integration.
//          Follows the engine namespace-with-free-functions pattern  (like
//          AudioSystem).  Provides async CRUD operations on Firestore documents
//          and a generic raw HTTP helper for advanced use.  Results are delivered
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
        ENGINE_API void update();     // Poll pending futures, dispatch callbacks (once per frame)
        ENGINE_API void shutdown();   // Cancel pending work, close session

        // ----------------------------------------------------------------
        // Connection status
        // ----------------------------------------------------------------
        ENGINE_API bool is_initialized();
        ENGINE_API bool is_configured();  // True if project_id & api_key are set

        // ----------------------------------------------------------------
        // Firestore CRUD  (all async — callback fires on the main thread)
        // ----------------------------------------------------------------

        // GET a single document.
        // Callback receives flat JSON: {"name":"Alice","score":100}
        ENGINE_API void get_document(
            const std::string& collection,
            const std::string& document_id,
            Callback on_complete);

        // CREATE or OVERWRITE a document.
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

        // LIST all documents in a collection.
        // Callback receives a JSON array: [{"_id":"doc1","name":"Alice"}, ...]
        ENGINE_API void query_collection(
            const std::string& collection,
            Callback on_complete);

        // ----------------------------------------------------------------
        // Generic raw HTTP request (for any REST endpoint)
        // ----------------------------------------------------------------
        ENGINE_API void raw_request(
            const std::string& method,   // "GET", "POST", "PATCH", "DELETE"
            const std::string& url,
            const std::string& body,
            Callback on_complete);

        // ----------------------------------------------------------------
        // How many requests are currently in flight
        // ----------------------------------------------------------------
        ENGINE_API size_t pending_count();

    } // namespace Firebase
} // namespace Engine
