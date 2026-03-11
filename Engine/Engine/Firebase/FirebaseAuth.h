//-----------------------------------------------------------------------------------
// file:    FirebaseAuth.h
// project: Monkey Stole My GPA
//
// brief:   Google Sign-In via OAuth 2.0 for Firebase Realtime Database.
//          Uses a local loopback HTTP server to receive the OAuth redirect,
//          exchanges the authorization code for a Firebase ID token, and
//          provides the token for all outgoing RTDB requests.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../EngineExport.h"
#include <string>

namespace Engine
{
    namespace Firebase
    {
        namespace Auth
        {
            // ----------------------------------------------------------------
            // Auth state
            // ----------------------------------------------------------------
            enum class AuthState
            {
                NotAuthenticated,   // No user signed in
                Pending,            // OAuth flow in progress (waiting for browser)
                Authenticated,      // Valid Firebase ID token obtained
                Error               // Something went wrong
            };

            // ----------------------------------------------------------------
            // Lifecycle — called from Core::init / Core::run / Core::exit
            // ----------------------------------------------------------------
            ENGINE_API bool init();       // Load settings (api_key, client_id, client_secret)
            ENGINE_API void update();     // Check token expiry, refresh if needed
            ENGINE_API void shutdown();   // Clean up

            // ----------------------------------------------------------------
            // Actions
            // ----------------------------------------------------------------
            ENGINE_API void sign_in_with_google();   // Opens browser, starts OAuth flow
            ENGINE_API void sign_out();

            // ----------------------------------------------------------------
            // State queries
            // ----------------------------------------------------------------
            ENGINE_API AuthState    get_state();
            ENGINE_API std::string  get_id_token();       // Firebase ID token for ?auth=
            ENGINE_API std::string  get_uid();
            ENGINE_API std::string  get_display_name();
            ENGINE_API std::string  get_email();
            ENGINE_API std::string  get_error();           // Last error message

            // Convenience
            ENGINE_API bool is_authenticated();

        } // namespace Auth
    } // namespace Firebase
} // namespace Engine
