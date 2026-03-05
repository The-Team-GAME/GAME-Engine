//-----------------------------------------------------------------------------------
// file:    FirebaseHTTP.cpp
// project: Monkey Stole My GPA
//
// brief:   WinHTTP implementation for HTTPS requests. Uses the Windows-native
//          WinHTTP API so there are zero external dependencies — TLS is handled
//          by the OS. All functions are synchronous; the manager layer handles
//          async dispatch via std::async.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#include "FirebaseHTTP.h"
#include "../Logger.h"

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

namespace Engine
{
    namespace Firebase
    {
        namespace HTTP
        {
            // ----------------------------------------------------------------
            // Module-level WinHTTP session handle
            // ----------------------------------------------------------------
            static HINTERNET g_session = nullptr;

            bool open_session(const std::string& user_agent)
            {
                if (g_session) return true; // Already open

                // Convert user agent to wide string
                std::wstring wide_ua(user_agent.begin(), user_agent.end());
                g_session = WinHttpOpen(
                    wide_ua.c_str(),
                    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                    WINHTTP_NO_PROXY_NAME,
                    WINHTTP_NO_PROXY_BYPASS,
                    0  // synchronous
                );

                if (!g_session) {
                    Log::error("[Firebase HTTP] WinHttpOpen failed, error: {}", GetLastError());
                    return false;
                }

                // Set reasonable timeouts (connect 10s, send 15s, receive 15s, resolve 5s)
                WinHttpSetTimeouts(g_session, 5000, 10000, 15000, 15000);

                return true;
            }

            void close_session()
            {
                if (g_session) {
                    WinHttpCloseHandle(g_session);
                    g_session = nullptr;
                }
            }

            // ----------------------------------------------------------------
            // Internal: parse a URL and execute an HTTP request
            // ----------------------------------------------------------------
            struct UrlParts
            {
                std::wstring host;
                std::wstring path;     // includes query string
                INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;
                bool is_https = true;
            };

            static bool parse_url(const std::string& url, UrlParts& out)
            {
                // Convert to wide
                std::wstring wide_url(url.begin(), url.end());

                URL_COMPONENTS components{};
                components.dwStructSize = sizeof(components);

                // Let WinHTTP allocate pointers into our wide_url string
                components.dwHostNameLength = (DWORD)-1;
                components.dwUrlPathLength = (DWORD)-1;
                components.dwExtraInfoLength = (DWORD)-1;

                if (!WinHttpCrackUrl(wide_url.c_str(), 0, 0, &components)) {
                    Log::error("[Firebase HTTP] Failed to parse URL: {}", url);
                    return false;
                }

                out.host = std::wstring(components.lpszHostName, components.dwHostNameLength);
                out.path = std::wstring(components.lpszUrlPath, components.dwUrlPathLength);
                // Append extra info (query string) if present
                if (components.dwExtraInfoLength > 0) {
                    out.path += std::wstring(components.lpszExtraInfo, components.dwExtraInfoLength);
                }
                out.port = components.nPort;
                out.is_https = (components.nScheme == INTERNET_SCHEME_HTTPS);

                return true;
            }

            static HttpResponse send_request(
                const std::string& method,
                const std::string& url,
                const std::string& body)
            {
                HttpResponse response;

                if (!g_session) {
                    response.body = "HTTP session not initialized";
                    return response;
                }

                UrlParts parts;
                if (!parse_url(url, parts)) {
                    response.body = "Failed to parse URL: " + url;
                    return response;
                }

                // Convert method to wide
                std::wstring wide_method(method.begin(), method.end());

                // Open connection to host
                HINTERNET connection = WinHttpConnect(
                    g_session,
                    parts.host.c_str(),
                    parts.port,
                    0
                );

                if (!connection) {
                    response.body = "WinHttpConnect failed, error: " + std::to_string(GetLastError());
                    Log::error("[Firebase HTTP] {}", response.body);
                    return response;
                }

                // Open the request
                DWORD flags = parts.is_https ? WINHTTP_FLAG_SECURE : 0;
                HINTERNET request = WinHttpOpenRequest(
                    connection,
                    wide_method.c_str(),
                    parts.path.c_str(),
                    nullptr,             // HTTP/1.1
                    WINHTTP_NO_REFERER,
                    WINHTTP_DEFAULT_ACCEPT_TYPES,
                    flags
                );

                if (!request) {
                    response.body = "WinHttpOpenRequest failed, error: " + std::to_string(GetLastError());
                    Log::error("[Firebase HTTP] {}", response.body);
                    WinHttpCloseHandle(connection);
                    return response;
                }

                // Add Content-Type header for requests with a body
                std::wstring headers = L"Content-Type: application/json\r\n";

                // Send the request
                LPVOID body_ptr = body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.data();
                DWORD body_len = static_cast<DWORD>(body.size());

                BOOL sent = WinHttpSendRequest(
                    request,
                    headers.c_str(),
                    (DWORD)-1L,    // auto-calc header length
                    body_ptr,
                    body_len,
                    body_len,
                    0              // context
                );

                if (!sent) {
                    response.body = "WinHttpSendRequest failed, error: " + std::to_string(GetLastError());
                    Log::error("[Firebase HTTP] {}", response.body);
                    WinHttpCloseHandle(request);
                    WinHttpCloseHandle(connection);
                    return response;
                }

                // Wait for the response
                if (!WinHttpReceiveResponse(request, nullptr)) {
                    response.body = "WinHttpReceiveResponse failed, error: " + std::to_string(GetLastError());
                    Log::error("[Firebase HTTP] {}", response.body);
                    WinHttpCloseHandle(request);
                    WinHttpCloseHandle(connection);
                    return response;
                }

                // Read status code
                DWORD status_code = 0;
                DWORD size = sizeof(status_code);
                WinHttpQueryHeaders(
                    request,
                    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    &status_code,
                    &size,
                    WINHTTP_NO_HEADER_INDEX
                );
                response.status_code = static_cast<int>(status_code);

                // Read response body
                std::string result_body;
                DWORD bytes_available = 0;
                do {
                    bytes_available = 0;
                    if (!WinHttpQueryDataAvailable(request, &bytes_available)) break;
                    if (bytes_available == 0) break;

                    std::vector<char> buffer(bytes_available);
                    DWORD bytes_read = 0;
                    if (WinHttpReadData(request, buffer.data(), bytes_available, &bytes_read)) {
                        result_body.append(buffer.data(), bytes_read);
                    }
                } while (bytes_available > 0);

                response.body = result_body;
                response.success = (status_code >= 200 && status_code < 300);

                // Cleanup
                WinHttpCloseHandle(request);
                WinHttpCloseHandle(connection);

                return response;
            }

            // ----------------------------------------------------------------
            // Public API
            // ----------------------------------------------------------------
            HttpResponse get(const std::string& url)
            {
                return send_request("GET", url, "");
            }

            HttpResponse post(const std::string& url, const std::string& body)
            {
                return send_request("POST", url, body);
            }

            HttpResponse patch(const std::string& url, const std::string& body)
            {
                return send_request("PATCH", url, body);
            }

            HttpResponse del(const std::string& url)
            {
                return send_request("DELETE", url, "");
            }

        } // namespace HTTP
    } // namespace Firebase
} // namespace Engine
