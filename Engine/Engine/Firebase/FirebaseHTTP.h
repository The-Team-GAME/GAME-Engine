//-----------------------------------------------------------------------------------
// file:    FirebaseHTTP.h
// project: Monkey Stole My GPA
//
// brief:   Low-level WinHTTP wrapper for making HTTPS requests to Firebase
//          Realtime Database REST API. Each function is synchronous — the caller
//          (FirebaseManager) is responsible for running them on background threads.
//          Also provides streaming support for SSE (Server-Sent Events).
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "FirebaseTypes.h"
#include <string>

// Forward-declare the WinHTTP handle type so callers don't need <winhttp.h>
typedef void* HINTERNET;

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
            HttpResponse put(const std::string& url, const std::string& body);
            HttpResponse del(const std::string& url);   // "delete" is a keyword

            // ----------------------------------------------------------------
            // SSE streaming support
            // ----------------------------------------------------------------

            // Holds the WinHTTP handles for an open SSE stream
            struct StreamHandle
            {
                HINTERNET connection = nullptr;
                HINTERNET request    = nullptr;
            };

            // Open an SSE stream: sends GET with Accept: text/event-stream,
            // receives headers, but does NOT read the body.
            // Returns a valid StreamHandle on success (both handles non-null).
            StreamHandle open_stream(const std::string& url);

            // Read a chunk of data from an open stream.
            // Returns number of bytes read (0 = no data yet or connection closed).
            int read_stream_chunk(HINTERNET request, char* buffer, int buffer_size);

            // Close a stream and release its handles.
            void close_stream(StreamHandle& handle);

        } // namespace HTTP
    } // namespace Firebase
} // namespace Engine
