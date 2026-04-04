//-----------------------------------------------------------------------------------
// file:    FirebaseTypes.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Shared types for the Firebase Realtime Database integration.
//          Defines callback signatures, HTTP response structures, and pending
//          request tracking.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <functional>
#include <future>

namespace Engine
{
  namespace Firebase
  {
    // ----------------------------------------------------------------
    // Callback type: (success, response_body_or_error_message)
    // ----------------------------------------------------------------
    using Callback = std::function<void(bool success, const std::string& data)>;

    // ----------------------------------------------------------------
    // Raw HTTP response from WinHTTP
    // ----------------------------------------------------------------
    struct HttpResponse
    {
      int    status_code = 0;
      std::string body;
      bool   success = false;   // true when status_code is 2xx
    };

    // ----------------------------------------------------------------
    // A single in-flight async request awaiting completion
    // ----------------------------------------------------------------
    struct PendingRequest
    {
      std::future<HttpResponse> future;
      Callback on_complete;     // Invoked on the main thread when the future resolves
    };

  } // namespace Firebase
} // namespace Engine


