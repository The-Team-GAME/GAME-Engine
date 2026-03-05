//-----------------------------------------------------------------------------------
// file:    FirebaseHTTP.h
// project: Monkey Stole My GPA
//
// brief:   Low-level WinHTTP wrapper for making HTTPS requests to Firebase
//          Cloud Firestore REST API. Each function is synchronous — the caller
//          (FirebaseManager) is responsible for running them on background threads.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "FirebaseTypes.h"
#include <string>

namespace Engine
{
    namespace Firebase
    {
        namespace HTTP
        {
            // ----------------------------------------------------------------
            // Session lifecycle — call once from FirebaseManager::init/shutdown
            // ----------------------------------------------------------------
            bool open_session(const std::string& user_agent = "MonkeyEngine/1.0");
            void close_session();

            // ----------------------------------------------------------------
            // Synchronous HTTPS requests (blocking — run on worker threads)
            // ----------------------------------------------------------------
            HttpResponse get(const std::string& url);
            HttpResponse post(const std::string& url, const std::string& body);
            HttpResponse patch(const std::string& url, const std::string& body);
            HttpResponse del(const std::string& url);   // "delete" is a keyword

        } // namespace HTTP
    } // namespace Firebase
} // namespace Engine
