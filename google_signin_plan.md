# Google Sign-In Implementation Plan

## Overview

Add Google Sign-In (OAuth 2.0) to the engine's Firebase integration so that:
1. A game developer can authenticate via Google in the ImGui panel
2. An ID token is attached to all Firebase RTDB requests
3. Firebase Security Rules can enforce `auth != null`
4. Sign-out clears credentials and subsequent requests are rejected by rules

---

## Architecture

```
┌──────────────────────────────────────────────────────────┐
│                    ImGui FirebasePanel                    │
│  ┌────────────────────────────────────────────────────┐  │
│  │  Authentication Section                            │  │
│  │  [Sign in with Google]  or  "Signed in as Alice"   │  │
│  │  Display name · Email · UID · [Sign Out]           │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────┬───────────────────────────────────────┘
                   │ calls
┌──────────────────▼───────────────────────────────────────┐
│              FirebaseAuth (new module)                    │
│  ┌─────────────────────┐  ┌────────────────────────────┐ │
│  │ Local HTTP Listener  │  │ Token Manager              │ │
│  │ localhost:PORT       │  │ id_token, refresh_token    │ │
│  │ receives OAuth code  │  │ auto-refresh before expiry │ │
│  └─────────┬───────────┘  └────────────┬───────────────┘ │
│            │                           │                  │
│  1. Browser opens Google OAuth         │                  │
│  2. User consents                      │                  │
│  3. Redirect → localhost:PORT?code=X   │                  │
│  4. Exchange code for Google tokens    │                  │
│  5. Exchange Google token → Firebase   │                  │
│     ID token via REST API              │                  │
│  6. Store id_token + refresh_token ────┘                  │
└──────────────────────────────────────────────────────────┘
                   │ provides token
┌──────────────────▼───────────────────────────────────────┐
│              FirebaseHTTP (modified)                      │
│  Every request adds header:                              │
│    ?auth={id_token}  (RTDB REST auth parameter)          │
│  SSE streams also append ?auth={id_token}                │
└──────────────────────────────────────────────────────────┘
```

---

## Step-by-Step Plan

### Step 0 — Firebase Console Setup (manual, not code)
- Enable **Google** as a sign-in provider in Firebase Console → Authentication → Sign-in method
- Create an **OAuth 2.0 Client ID** (Desktop app type) in Google Cloud Console
- Note the `client_id` and `client_secret`
- Add `http://localhost` (with port) as an authorized redirect URI
- Update `firebase_settings.json` to include:
  ```json
  {
    "database_url": "https://monkeyengine-2ad5a-default-rtdb...",
    "api_key": "AIza...",
    "client_id": "XXXX.apps.googleusercontent.com",
    "client_secret": "GOCSPX-..."
  }
  ```

### Step 1 — FirebaseAuth Module (new files)

**New files:**
- `Engine/Engine/Firebase/FirebaseAuth.h`
- `Engine/Engine/Firebase/FirebaseAuth.cpp`

**Responsibilities:**
1. **Local loopback HTTP server** — Listens on `127.0.0.1:<dynamic_port>` to receive the OAuth redirect
2. **Browser launch** — Opens the Google OAuth consent screen in the user's default browser
3. **Authorization code exchange** — Exchanges the `code` for Google `access_token` + `id_token` via `https://oauth2.googleapis.com/token`
4. **Firebase token exchange** — Sends the Google `id_token` to Firebase Auth REST API (`https://identitytoolkit.googleapis.com/v1/accounts:signInWithIdp?key=API_KEY`) to get a Firebase `idToken`, `refreshToken`, `localId` (UID), `displayName`, `email`
5. **Token storage** — Holds current auth state in memory (not persisted to disk for security)
6. **Token refresh** — Firebase ID tokens expire after 1 hour. A background refresh using `https://securetoken.googleapis.com/v1/token?key=API_KEY` with the `refresh_token` keeps the session alive
7. **Sign-out** — Clears all tokens and user info

**Public API:**
```cpp
namespace Engine::Firebase::Auth
{
    // Lifecycle
    bool init(const std::string& api_key,
              const std::string& client_id,
              const std::string& client_secret);
    void update();    // Check token expiry, refresh if needed
    void shutdown();

    // Actions
    void sign_in_with_google();   // Opens browser, starts OAuth flow
    void sign_out();

    // State queries
    bool is_authenticated();
    std::string get_id_token();     // Current Firebase ID token (for HTTP requests)
    std::string get_uid();
    std::string get_display_name();
    std::string get_email();

    // Status for UI
    enum class AuthState { NotAuthenticated, Pending, Authenticated, Error };
    AuthState get_state();
    std::string get_error();        // Last error message if state == Error
}
```

### Step 2 — Local Loopback OAuth Receiver

**Implementation detail** — Use WinSock2 (already available on Windows) to create a minimal TCP server:

1. `socket()` → `bind(127.0.0.1, 0)` → `getsockname()` to get the OS-assigned port → `listen()`
2. Construct the Google OAuth URL:
   ```
   https://accounts.google.com/o/oauth2/v2/auth
     ?client_id={CLIENT_ID}
     &redirect_uri=http://127.0.0.1:{PORT}
     &response_type=code
     &scope=openid%20email%20profile
     &access_type=offline
   ```
3. Open URL in browser via `ShellExecuteW(nullptr, L"open", url, ...)`
4. `accept()` one connection on a background thread
5. Parse the HTTP GET request to extract `?code=...`
6. Send back an HTML response: "Sign-in successful! You can close this tab."
7. Close socket, proceed to token exchange

**Threading:** Run the listener on a detached background thread. Signal the main thread via an atomic flag + queued result.

### Step 3 — Token Exchange Flow

**Stage A: Google token exchange** (background thread)
```
POST https://oauth2.googleapis.com/token
Content-Type: application/x-www-form-urlencoded

code={AUTH_CODE}
&client_id={CLIENT_ID}
&client_secret={CLIENT_SECRET}
&redirect_uri=http://127.0.0.1:{PORT}
&grant_type=authorization_code
```
→ Response: `{ "access_token", "id_token", "refresh_token", "expires_in" }`

**Stage B: Firebase sign-in with Google credential** (background thread)
```
POST https://identitytoolkit.googleapis.com/v1/accounts:signInWithIdp?key={API_KEY}
Content-Type: application/json

{
  "postBody": "id_token={GOOGLE_ID_TOKEN}&providerId=google.com",
  "requestUri": "http://localhost",
  "returnIdpCredential": true,
  "returnSecureToken": true
}
```
→ Response: `{ "idToken", "refreshToken", "localId", "displayName", "email", "expiresIn" }`

Store: `idToken` (Firebase), `refreshToken`, `localId` (UID), `displayName`, `email`

### Step 4 — Token Refresh

- Firebase ID tokens expire in **3600 seconds** (1 hour)
- In `Auth::update()` (called each frame), check if token will expire within next 5 minutes
- If so, fire an async refresh:
  ```
  POST https://securetoken.googleapis.com/v1/token?key={API_KEY}
  Content-Type: application/x-www-form-urlencoded

  grant_type=refresh_token&refresh_token={REFRESH_TOKEN}
  ```
  → Response: `{ "id_token", "refresh_token", "expires_in" }`
- Update stored tokens atomically (mutex-protected)

### Step 5 — Inject Auth Token into HTTP Requests

**RTDB REST API** uses `?auth=TOKEN` query parameter (not Authorization header).

**Modify `FirebaseManager.cpp`:**

In the `rtdb_url()` helper, append `?auth={id_token}` if authenticated:
```cpp
static std::string rtdb_url(const std::string& collection, const std::string& doc_id = "")
{
    std::string url = g_base_url + "/" + collection;
    if (!doc_id.empty()) url += "/" + doc_id;
    url += ".json";

    // Append auth token if signed in
    std::string token = Auth::get_id_token();
    if (!token.empty()) {
        url += "?auth=" + token;
    }

    return url;
}
```

**Modify SSE watch URLs** in `FirebaseListener.cpp`:
- Same pattern — append `?auth={token}` to stream URLs
- On token refresh, reconnect active watches with new token

### Step 6 — FirebasePanel UI (Authentication Section)

Add a new collapsible section at the **top** of the Firebase panel, above Connection Status:

**Not Authenticated state:**
```
┌─ Authentication ──────────────────────────┐
│  Status: Not Authenticated (red dot)      │
│  [Sign in with Google]                    │
└───────────────────────────────────────────┘
```

**Pending state:**
```
┌─ Authentication ──────────────────────────┐
│  Status: Waiting for browser... (yellow)  │
│  Complete sign-in in your browser.        │
└───────────────────────────────────────────┘
```

**Authenticated state:**
```
┌─ Authentication ──────────────────────────┐
│  Status: Authenticated (green dot)        │
│  Name:  Alice Smith                       │
│  Email: alice@gmail.com                   │
│  UID:   abc123def456                      │
│  [Sign Out]                               │
└───────────────────────────────────────────┘
```

**Error state:**
```
┌─ Authentication ──────────────────────────┐
│  Status: Error (red dot)                  │
│  "Token exchange failed: invalid_grant"   │
│  [Retry Sign In]                          │
└───────────────────────────────────────────┘
```

### Step 7 — Integrate into Engine Lifecycle

**`Core.cpp` or wherever `Firebase::init()` is called:**
```cpp
// In init:
Firebase::init();
Firebase::Auth::init(api_key, client_id, client_secret);

// In update loop:
Firebase::update();
Firebase::Auth::update();   // Token refresh check

// In shutdown:
Firebase::Auth::shutdown();
Firebase::shutdown();
```

### Step 8 — Settings File Update

Update `firebase_settings.json` schema and loader:

```json
{
  "database_url": "https://monkeyengine-2ad5a-default-rtdb.asia-southeast1.firebasedatabase.app/",
  "api_key": "AIzaSy...",
  "client_id": "123456789.apps.googleusercontent.com",
  "client_secret": "GOCSPX-..."
}
```

Update `FirebaseSettings` struct and `SettingsManager` to load the new fields.

### Step 9 — Handle 401 / Permission Denied

When a request returns HTTP 401 or body contains `"error" : "Permission denied"`:
1. If authenticated → attempt token refresh → retry once
2. If not authenticated → surface error in Activity Log: "Authentication required"
3. If refresh fails → set auth state to Error, prompt re-sign-in

---

## File Changes Summary

| File                     | Change                                                  |
| ------------------------ | ------------------------------------------------------- |
| `FirebaseAuth.h`         | **NEW** — Auth module public API                        |
| `FirebaseAuth.cpp`       | **NEW** — OAuth flow, loopback server, token management |
| `FirebaseManager.cpp`    | **MODIFY** — Append `?auth=` to RTDB URLs               |
| `FirebaseListener.cpp`   | **MODIFY** — Append `?auth=` to SSE stream URLs         |
| `FirebasePanel.cpp`      | **MODIFY** — Add Authentication UI section              |
| `FirebaseTypes.h`        | **MODIFY** — Add auth-related types if needed           |
| `firebase_settings.json` | **MODIFY** — Add api_key, client_id, client_secret      |
| `Settings.h`             | **MODIFY** — Add new fields to FirebaseSettings struct  |
| `Core.cpp`               | **MODIFY** — Call Auth::init/update/shutdown            |
| `Engine.vcxproj`         | **MODIFY** — Add new .h/.cpp files to project           |

---

## Testing Checklist (matches rubric)

- [ ] Click "Sign in with Google" → browser opens OAuth consent screen
- [ ] Complete sign-in in browser → panel shows name, email, UID, green status
- [ ] Fetch a collection → succeeds (auth token attached automatically)
- [ ] Sign out → user info cleared, status returns to "Not Authenticated"
- [ ] Fetch a collection after sign-out → fails with permission error (if rules require auth)
- [ ] Token auto-refreshes after ~55 minutes without user action
- [ ] Error state shown if OAuth flow is cancelled or fails

---

## Dependencies

- **No new external libraries** — Uses WinHTTP (already used) + WinSock2 (built into Windows) + ShellExecuteW (Win32)
- Requires Firebase Console configuration (enable Google sign-in provider, create OAuth client)

---

## Security Considerations

- `client_secret` is stored in `firebase_settings.json` which ships with the engine — this is acceptable for desktop OAuth apps (Google classifies desktop apps as "public clients"), but the secret should not be committed to public repos
- Tokens are held in memory only, never written to disk
- Loopback listener binds to `127.0.0.1` only (not `0.0.0.0`) to prevent network exposure
- PKCE (Proof Key for Code Exchange) can be added as an enhancement for extra security
