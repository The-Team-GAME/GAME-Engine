//-----------------------------------------------------------------------------------
// file:    FirebaseAuth.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Google Sign-In implementation. Uses a local loopback TCP server
//          (WinSock2) to receive the OAuth 2.0 redirect, exchanges the
//          authorization code for a Firebase ID token via WinHTTP, and
//          handles automatic token refresh.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#include <shellapi.h>

#include "FirebaseAuth.h"
#include "FirebaseHTTP.h"
#include "../Logger.h"
#include "../FileIO/Settings.h"
#include "../FileIO/SettingsManager.h"

#include <rapidjson/document.h>

#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>

namespace Engine
{
  namespace Firebase
  {
    namespace Auth
    {
      // ================================================================
      // Internal state
      // ================================================================
      static std::mutex       g_mutex;
      static AuthState        g_state = AuthState::NotAuthenticated;
      static std::string      g_error;

      // Firebase credentials (loaded from settings)
      static std::string      g_api_key;
      static std::string      g_client_id;
      static std::string      g_client_secret;

      // User info (populated after sign-in)
      static std::string      g_id_token;
      static std::string      g_refresh_token;
      static std::string      g_uid;
      static std::string      g_display_name;
      static std::string      g_email;

      // Token expiry tracking
      static std::chrono::steady_clock::time_point g_token_expiry;
      static constexpr int REFRESH_MARGIN_SECONDS = 300; // Refresh 5 min before expiry

      // OAuth flow
      static std::thread      g_auth_thread;
      static std::atomic<bool> g_auth_thread_active{ false };

      // Pending result from background thread
      struct AuthResult
      {
        bool        success = false;
        std::string error;
        std::string id_token;
        std::string refresh_token;
        std::string uid;
        std::string display_name;
        std::string email;
        int         expires_in = 0;
      };
      static AuthResult       g_pending_result;
      static std::atomic<bool> g_result_ready{ false };

      // Refresh in progress
      static std::atomic<bool> g_refresh_active{ false };

      // ================================================================
      // WinSock helpers
      // ================================================================
      static bool g_winsock_inited = false;

      static bool init_winsock()
      {
        if (g_winsock_inited) return true;
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        {
          Log::error("[Firebase Auth] WSAStartup failed: {}", WSAGetLastError());
          return false;
        }
        g_winsock_inited = true;
        return true;
      }

      static void cleanup_winsock()
      {
        if (g_winsock_inited)
        {
          WSACleanup();
          g_winsock_inited = false;
        }
      }

      // ================================================================
      // URL-encode a string (for form data)
      // ================================================================
      static std::string url_encode(const std::string& value)
      {
        std::ostringstream encoded;
        encoded.fill('0');
        encoded << std::hex;
        for (unsigned char c : value)
        {
          if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            encoded << c;
          else
            encoded << '%' << std::setw(2) << static_cast<int>(c);
        }
        return encoded.str();
      }

      // ================================================================
      // URL-decode a string (for parsing OAuth redirect parameters)
      // ================================================================
      static std::string url_decode(const std::string& value)
      {
        std::string result;
        result.reserve(value.size());
        for (size_t i = 0; i < value.size(); ++i)
        {
          if (value[i] == '%' && i + 2 < value.size())
          {
            char hex[3] = { value[i + 1], value[i + 2], '\0' };
            result += static_cast<char>(std::strtol(hex, nullptr, 16));
            i += 2;
          }
          else if (value[i] == '+')
            result += ' ';
          else
            result += value[i];
        }
        return result;
      }

      // ================================================================
      // POST helper using WinHTTP (form-urlencoded or JSON)
      // ================================================================
      static HttpResponse post_form(const std::string& url, const std::string& body)
      {
        // Use FirebaseHTTP's existing infrastructure but we need a custom
        // content-type for form data. For simplicity, we'll use the
        // existing HTTP::post which sends application/json content-type.
        // For the Google token endpoint we need application/x-www-form-urlencoded.
        // So we implement a minimal POST here.

        HttpResponse response;

        // Parse URL
        std::wstring wide_url(url.begin(), url.end());
        URL_COMPONENTS components{};
        components.dwStructSize = sizeof(components);
        components.dwHostNameLength = (DWORD)-1;
        components.dwUrlPathLength = (DWORD)-1;
        components.dwExtraInfoLength = (DWORD)-1;

        if (!WinHttpCrackUrl(wide_url.c_str(), 0, 0, &components))
        {
          response.body = "Failed to parse URL";
          return response;
        }

        std::wstring host(components.lpszHostName, components.dwHostNameLength);
        std::wstring path(components.lpszUrlPath, components.dwUrlPathLength);
        if (components.dwExtraInfoLength > 0)
          path += std::wstring(components.lpszExtraInfo, components.dwExtraInfoLength);

        // We need the global WinHTTP session from FirebaseHTTP
        // Use HTTP::post for JSON endpoints, but for form data we need different headers.
        // Actually, let's just do a direct WinHTTP call using the session that's already open.
        // Hmm, the session handle is static in FirebaseHTTP.cpp. Let's use a fresh session
        // for auth requests to avoid coupling. Actually, simpler: just use HTTP::post for JSON
        // endpoints and build a minimal request for form endpoints.

        HINTERNET session = WinHttpOpen(
          L"MonkeyEngine-Auth/1.0",
          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
          WINHTTP_NO_PROXY_NAME,
          WINHTTP_NO_PROXY_BYPASS,
          0
        );
        if (!session) { response.body = "WinHttpOpen failed"; return response; }

        HINTERNET connection = WinHttpConnect(
          session, host.c_str(), components.nPort, 0);
        if (!connection)
        {
          response.body = "WinHttpConnect failed";
          WinHttpCloseHandle(session);
          return response;
        }

        DWORD flags = (components.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET request = WinHttpOpenRequest(
          connection, L"POST", path.c_str(),
          nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!request)
        {
          response.body = "WinHttpOpenRequest failed";
          WinHttpCloseHandle(connection);
          WinHttpCloseHandle(session);
          return response;
        }

        std::wstring headers = L"Content-Type: application/x-www-form-urlencoded\r\n";
        BOOL sent = WinHttpSendRequest(
          request, headers.c_str(), (DWORD)-1L,
          (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);

        if (!sent)
        {
          response.body = "WinHttpSendRequest failed";
          WinHttpCloseHandle(request);
          WinHttpCloseHandle(connection);
          WinHttpCloseHandle(session);
          return response;
        }

        if (!WinHttpReceiveResponse(request, nullptr))
        {
          response.body = "WinHttpReceiveResponse failed";
          WinHttpCloseHandle(request);
          WinHttpCloseHandle(connection);
          WinHttpCloseHandle(session);
          return response;
        }

        DWORD status_code = 0;
        DWORD sz = sizeof(status_code);
        WinHttpQueryHeaders(request,
          WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
          WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &sz, WINHTTP_NO_HEADER_INDEX);
        response.status_code = static_cast<int>(status_code);

        std::string result_body;
        DWORD bytes_available = 0;
        do {
          bytes_available = 0;
          if (!WinHttpQueryDataAvailable(request, &bytes_available)) break;
          if (bytes_available == 0) break;
          std::vector<char> buf(bytes_available);
          DWORD bytes_read = 0;
          if (WinHttpReadData(request, buf.data(), bytes_available, &bytes_read))
            result_body.append(buf.data(), bytes_read);
        } while (bytes_available > 0);

        response.body = result_body;
        response.success = (status_code >= 200 && status_code < 300);

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);

        return response;
      }

      // ================================================================
      // POST helper for JSON content-type
      // ================================================================
      static HttpResponse post_json(const std::string& url, const std::string& body)
      {
        // Reuse the FirebaseHTTP post which sets Content-Type: application/json
        return HTTP::post(url, body);
      }

      // ================================================================
      // Token exchange: auth code → Google tokens → Firebase ID token
      // ================================================================
      static AuthResult exchange_tokens(const std::string& auth_code, int port)
      {
        AuthResult result;

        // --- Stage A: Exchange auth code for Google tokens ---
        std::string redirect_uri = "http://127.0.0.1:" + std::to_string(port);
        std::string form_body =
          "code=" + url_encode(auth_code) +
          "&client_id=" + url_encode(g_client_id) +
          "&client_secret=" + url_encode(g_client_secret) +
          "&redirect_uri=" + url_encode(redirect_uri) +
          "&grant_type=authorization_code";

        Log::info("[Firebase Auth] Exchanging auth code for Google tokens...");
        HttpResponse google_resp = post_form("https://oauth2.googleapis.com/token", form_body);

        if (!google_resp.success)
        {
          result.error = "Google token exchange failed (HTTP " +
            std::to_string(google_resp.status_code) + "): " + google_resp.body;
          Log::error("[Firebase Auth] {}", result.error);
          return result;
        }

        // Parse Google response
        rapidjson::Document google_doc;
        google_doc.Parse(google_resp.body.c_str());
        if (google_doc.HasParseError() || !google_doc.IsObject())
        {
          result.error = "Failed to parse Google token response";
          Log::error("[Firebase Auth] {}", result.error);
          return result;
        }

        std::string google_id_token;
        if (google_doc.HasMember("id_token") && google_doc["id_token"].IsString())
          google_id_token = google_doc["id_token"].GetString();

        if (google_id_token.empty())
        {
          result.error = "No id_token in Google response";
          Log::error("[Firebase Auth] {}", result.error);
          return result;
        }

        // --- Stage B: Exchange Google ID token for Firebase ID token ---
        std::string firebase_url =
          "https://identitytoolkit.googleapis.com/v1/accounts:signInWithIdp?key=" + g_api_key;

        std::string firebase_body =
          "{\"postBody\":\"id_token=" + google_id_token + "&providerId=google.com\","
          "\"requestUri\":\"http://localhost\","
          "\"returnIdpCredential\":true,"
          "\"returnSecureToken\":true}";

        Log::info("[Firebase Auth] Exchanging Google token for Firebase ID token...");
        HttpResponse fb_resp = post_json(firebase_url, firebase_body);

        if (!fb_resp.success)
        {
          result.error = "Firebase signInWithIdp failed (HTTP " +
            std::to_string(fb_resp.status_code) + "): " + fb_resp.body;
          Log::error("[Firebase Auth] {}", result.error);
          return result;
        }

        rapidjson::Document fb_doc;
        fb_doc.Parse(fb_resp.body.c_str());
        if (fb_doc.HasParseError() || !fb_doc.IsObject())
        {
          result.error = "Failed to parse Firebase signInWithIdp response";
          Log::error("[Firebase Auth] {}", result.error);
          return result;
        }

        // Extract fields
        if (fb_doc.HasMember("idToken") && fb_doc["idToken"].IsString())
          result.id_token = fb_doc["idToken"].GetString();
        if (fb_doc.HasMember("refreshToken") && fb_doc["refreshToken"].IsString())
          result.refresh_token = fb_doc["refreshToken"].GetString();
        if (fb_doc.HasMember("localId") && fb_doc["localId"].IsString())
          result.uid = fb_doc["localId"].GetString();
        if (fb_doc.HasMember("displayName") && fb_doc["displayName"].IsString())
          result.display_name = fb_doc["displayName"].GetString();
        if (fb_doc.HasMember("email") && fb_doc["email"].IsString())
          result.email = fb_doc["email"].GetString();
        if (fb_doc.HasMember("expiresIn") && fb_doc["expiresIn"].IsString())
          result.expires_in = std::atoi(fb_doc["expiresIn"].GetString());

        if (result.id_token.empty())
        {
          result.error = "No idToken in Firebase response";
          Log::error("[Firebase Auth] {}", result.error);
          return result;
        }

        result.success = true;
        Log::info("[Firebase Auth] Signed in as {} ({})", result.display_name, result.email);
        return result;
      }

      // ================================================================
      // Token refresh
      // ================================================================
      static void do_refresh()
      {
        std::string current_refresh;
        {
          std::lock_guard<std::mutex> lock(g_mutex);
          current_refresh = g_refresh_token;
        }

        if (current_refresh.empty())
        {
          g_refresh_active.store(false);
          return;
        }

        std::string url = "https://securetoken.googleapis.com/v1/token?key=" + g_api_key;
        std::string body = "grant_type=refresh_token&refresh_token=" + url_encode(current_refresh);

        HttpResponse resp = post_form(url, body);

        std::lock_guard<std::mutex> lock(g_mutex);
        g_refresh_active.store(false);

        if (!resp.success)
        {
          Log::warning("[Firebase Auth] Token refresh failed (HTTP {}): {}",
            resp.status_code, resp.body);
          return;
        }

        rapidjson::Document doc;
        doc.Parse(resp.body.c_str());
        if (doc.HasParseError() || !doc.IsObject()) return;

        if (doc.HasMember("id_token") && doc["id_token"].IsString())
          g_id_token = doc["id_token"].GetString();
        if (doc.HasMember("refresh_token") && doc["refresh_token"].IsString())
          g_refresh_token = doc["refresh_token"].GetString();

        int expires_in = 3600;
        if (doc.HasMember("expires_in") && doc["expires_in"].IsString())
          expires_in = std::atoi(doc["expires_in"].GetString());

        g_token_expiry = std::chrono::steady_clock::now() +
          std::chrono::seconds(expires_in);

        Log::info("[Firebase Auth] Token refreshed, expires in {}s", expires_in);
      }

      // ================================================================
      // OAuth flow thread
      // ================================================================
      static void auth_thread_func()
      {
        AuthResult result;

        // 1. Start loopback server
        int port = 0;
        std::string redirect_uri;

        // Start server and get port
        // We need to open the browser AFTER we know the port
        // but BEFORE we accept the connection.
        // So: create socket, bind, listen, get port, open browser, then accept.

        SOCKET server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_sock == INVALID_SOCKET)
        {
          result.error = "socket() failed: " + std::to_string(WSAGetLastError());
          Log::error("[Firebase Auth] {}", result.error);
          g_pending_result = result;
          g_result_ready.store(true);
          g_auth_thread_active.store(false);
          return;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = 0;
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (bind(server_sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
          result.error = "bind() failed: " + std::to_string(WSAGetLastError());
          closesocket(server_sock);
          g_pending_result = result;
          g_result_ready.store(true);
          g_auth_thread_active.store(false);
          return;
        }

        sockaddr_in bound{};
        int bound_len = sizeof(bound);
        getsockname(server_sock, (sockaddr*)&bound, &bound_len);
        port = ntohs(bound.sin_port);

        if (listen(server_sock, 1) == SOCKET_ERROR)
        {
          result.error = "listen() failed: " + std::to_string(WSAGetLastError());
          closesocket(server_sock);
          g_pending_result = result;
          g_result_ready.store(true);
          g_auth_thread_active.store(false);
          return;
        }

        Log::info("[Firebase Auth] Loopback on 127.0.0.1:{}", port);

        // 2. Open browser with Google OAuth URL
        redirect_uri = "http://127.0.0.1:" + std::to_string(port);
        std::string oauth_url =
          "https://accounts.google.com/o/oauth2/v2/auth"
          "?client_id=" + url_encode(g_client_id) +
          "&redirect_uri=" + url_encode(redirect_uri) +
          "&response_type=code"
          "&scope=openid%20email%20profile"
          "&access_type=offline"
          "&prompt=consent";

        std::wstring wide_url(oauth_url.begin(), oauth_url.end());
        ShellExecuteW(nullptr, L"open", wide_url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

        // 3. Wait for redirect (with timeout)
        DWORD timeout_ms = 120000;
        setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO,
          (const char*)&timeout_ms, sizeof(timeout_ms));

        SOCKET client = accept(server_sock, nullptr, nullptr);
        if (client == INVALID_SOCKET)
        {
          result.error = "OAuth timed out — no response from browser";
          Log::error("[Firebase Auth] {}", result.error);
          closesocket(server_sock);
          g_pending_result = result;
          g_result_ready.store(true);
          g_auth_thread_active.store(false);
          return;
        }

        // Read HTTP request
        char buf[4096]{};
        int received = recv(client, buf, sizeof(buf) - 1, 0);
        std::string request_str;
        if (received > 0)
          request_str.assign(buf, received);

        // Send response HTML
        const char* html =
          "<!DOCTYPE html><html><body style='font-family:sans-serif;text-align:center;padding:50px'>"
          "<h2>Sign-in successful!</h2>"
          "<p>You can close this tab and return to the engine.</p>"
          "</body></html>";

        std::ostringstream http_resp;
        http_resp << "HTTP/1.1 200 OK\r\n"
          << "Content-Type: text/html\r\n"
          << "Content-Length: " << strlen(html) << "\r\n"
          << "Connection: close\r\n\r\n"
          << html;
        std::string resp_str = http_resp.str();
        send(client, resp_str.c_str(), static_cast<int>(resp_str.size()), 0);

        // WinSock shutdown for send direction
        ::shutdown(client, SD_SEND);
        closesocket(client);
        closesocket(server_sock);

        // 4. Extract auth code
        std::string auth_code;
        size_t code_pos = request_str.find("code=");
        if (code_pos != std::string::npos)
        {
          size_t start = code_pos + 5;
          size_t end = request_str.find_first_of("& \r\n", start);
          if (end == std::string::npos) end = request_str.size();
          auth_code = url_decode(request_str.substr(start, end - start));
        }

        if (auth_code.empty())
        {
          // Check for error
          size_t err_pos = request_str.find("error=");
          if (err_pos != std::string::npos)
          {
            size_t start = err_pos + 6;
            size_t end = request_str.find_first_of("& \r\n", start);
            if (end == std::string::npos) end = request_str.size();
            result.error = "OAuth denied: " + request_str.substr(start, end - start);
          }
          else
          {
            result.error = "No authorization code received";
          }
          Log::error("[Firebase Auth] {}", result.error);
          g_pending_result = result;
          g_result_ready.store(true);
          g_auth_thread_active.store(false);
          return;
        }

        // 5. Exchange tokens
        result = exchange_tokens(auth_code, port);

        g_pending_result = result;
        g_result_ready.store(true);
        g_auth_thread_active.store(false);
      }

      // ================================================================
      // Public API
      // ================================================================

      bool init()
      {
        const auto& settings =
          Engine::FileIO::SettingsManager<Engine::FileIO::FirebaseSettings>::get();

        g_api_key = settings.api_key;
        g_client_id = settings.client_id;
        g_client_secret = settings.client_secret;

        if (g_api_key.empty() || g_client_id.empty() || g_client_secret.empty())
        {
          Log::warning("[Firebase Auth] OAuth credentials not configured in firebase_settings.json"
            " — Google Sign-In disabled");
          return true; // Not a hard failure, auth is optional
        }

        if (!init_winsock())
          return false;

        Log::info("[Firebase Auth] Initialized (OAuth credentials loaded)");
        return true;
      }

      void update()
      {
        // Check if auth thread produced a result
        if (g_result_ready.load())
        {
          g_result_ready.store(false);

          std::lock_guard<std::mutex> lock(g_mutex);
          if (g_pending_result.success)
          {
            g_id_token = g_pending_result.id_token;
            g_refresh_token = g_pending_result.refresh_token;
            g_uid = g_pending_result.uid;
            g_display_name = g_pending_result.display_name;
            g_email = g_pending_result.email;
            g_token_expiry = std::chrono::steady_clock::now() +
              std::chrono::seconds(g_pending_result.expires_in > 0
                ? g_pending_result.expires_in : 3600);
            g_state = AuthState::Authenticated;
            g_error.clear();
          }
          else
          {
            g_state = AuthState::Error;
            g_error = g_pending_result.error;
          }
        }

        // Token refresh check (only if authenticated)
        if (g_state == AuthState::Authenticated && !g_refresh_active.load())
        {
          auto now = std::chrono::steady_clock::now();
          auto margin = std::chrono::seconds(REFRESH_MARGIN_SECONDS);
          if (now + margin >= g_token_expiry)
          {
            g_refresh_active.store(true);
            std::thread(do_refresh).detach();
          }
        }
      }

      void shutdown()
      {
        // Wait for auth thread if active
        if (g_auth_thread.joinable())
          g_auth_thread.join();

        g_state = AuthState::NotAuthenticated;
        g_id_token.clear();
        g_refresh_token.clear();
        g_uid.clear();
        g_display_name.clear();
        g_email.clear();
        g_error.clear();

        cleanup_winsock();
        Log::info("[Firebase Auth] Shutdown");
      }

      void sign_in_with_google()
      {
        if (g_api_key.empty() || g_client_id.empty() || g_client_secret.empty())
        {
          std::lock_guard<std::mutex> lock(g_mutex);
          g_state = AuthState::Error;
          g_error = "OAuth credentials not configured in firebase_settings.json";
          return;
        }

        if (g_auth_thread_active.load())
          return; // Already in progress

        // Clean up previous thread
        if (g_auth_thread.joinable())
          g_auth_thread.join();

        {
          std::lock_guard<std::mutex> lock(g_mutex);
          g_state = AuthState::Pending;
          g_error.clear();
        }

        g_auth_thread_active.store(true);
        g_auth_thread = std::thread(auth_thread_func);
      }

      void sign_out()
      {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_state = AuthState::NotAuthenticated;
        g_id_token.clear();
        g_refresh_token.clear();
        g_uid.clear();
        g_display_name.clear();
        g_email.clear();
        g_error.clear();
        Log::info("[Firebase Auth] Signed out");
      }

      AuthState get_state()
      {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_state;
      }

      std::string get_id_token()
      {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_id_token;
      }

      std::string get_uid()
      {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_uid;
      }

      std::string get_display_name()
      {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_display_name;
      }

      std::string get_email()
      {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_email;
      }

      std::string get_error()
      {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_error;
      }

      bool is_authenticated()
      {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_state == AuthState::Authenticated;
      }

    } // namespace Auth
  } // namespace Firebase
} // namespace Engine


