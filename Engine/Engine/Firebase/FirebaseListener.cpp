//-----------------------------------------------------------------------------------
// file:    FirebaseListener.cpp
// project: Monkey Stole My GPA
//
// brief:   SSE listener implementation. Each watch target gets a dedicated thread
//          that opens a streaming GET request with "Accept: text/event-stream" and
//          parses the standard SSE format (event:/data: lines). Events are queued
//          in a thread-safe buffer and drained on the main thread.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "FirebaseListener.h"
#include "FirebaseHTTP.h"
#include "../Logger.h"

#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <string>
#include <algorithm>

namespace Engine
{
  namespace Firebase
  {
    namespace Listener
    {
      // ================================================================
      // Internal types
      // ================================================================

      struct SSEEvent
      {
        int         watch_id;
        std::string json;       // {"event":"put","path":"/...","data":{...}}
      };

      struct WatchTarget
      {
        int                 id;
        std::string         url;
        Callback            on_change;
        std::thread         thread;
        std::atomic<bool>   cancel{ false };
      };

      // ================================================================
      // Internal state
      // ================================================================
      static std::mutex                              g_mutex;
      static std::vector<std::unique_ptr<WatchTarget>> g_watches;
      static std::vector<SSEEvent>                   g_event_queue;
      static int                                     g_next_id = 0;

      // ================================================================
      // SSE line parser
      // ================================================================

      // Parse the SSE stream. SSE protocol:
      //   event: <type>\n
      //   data: <json>\n
      //   \n              <-- blank line = dispatch
      static void reader_thread(WatchTarget* target)
      {
        int backoff_ms = 1000; // Reconnection backoff
        constexpr int MAX_BACKOFF_MS = 30000;

        while (!target->cancel.load())
        {
          HTTP::StreamHandle stream = HTTP::open_stream(target->url);
          if (!stream.request)
          {
            if (target->cancel.load()) break;
            Log::warning("[Firebase Listener] Watch {} failed to connect, retrying in {}ms",
              target->id, backoff_ms);
            // Sleep in small increments so we can respond to cancel
            for (int slept = 0; slept < backoff_ms && !target->cancel.load(); slept += 100)
              std::this_thread::sleep_for(std::chrono::milliseconds(100));
            backoff_ms = (std::min)(backoff_ms * 2, MAX_BACKOFF_MS);
            continue;
          }

          // Connected successfully — reset backoff
          backoff_ms = 1000;
          Log::info("[Firebase Listener] Watch {} connected to {}", target->id, target->url);

          // Accumulate partial reads into a line buffer
          std::string line_buffer;
          std::string current_event;
          std::string current_data;
          char chunk[4096];

          while (!target->cancel.load())
          {
            int n = HTTP::read_stream_chunk(stream.request, chunk, sizeof(chunk));
            if (n < 0)
            {
              // Error — connection lost
              Log::warning("[Firebase Listener] Watch {} read error, reconnecting", target->id);
              break;
            }
            if (n == 0)
            {
              // No data yet — small sleep to avoid busy-spin
              std::this_thread::sleep_for(std::chrono::milliseconds(50));
              continue;
            }

            line_buffer.append(chunk, n);

            // Process complete lines
            size_t pos = 0;
            while (pos < line_buffer.size())
            {
              size_t eol = line_buffer.find('\n', pos);
              if (eol == std::string::npos) break; // Incomplete line

              std::string line = line_buffer.substr(pos, eol - pos);
              // Strip trailing \r if present
              if (!line.empty() && line.back() == '\r') line.pop_back();
              pos = eol + 1;

              if (line.empty())
              {
                // Blank line = dispatch event
                if (!current_event.empty() && current_event != "keep-alive")
                {
                  // Build event JSON: {"event":"put","path":"/...","data":{...}}
                  // The data field from RTDB is already: {"path":"/...","data":{...}}
                  // We wrap it: {"event":"<type>", ...rest of data}
                  std::string event_json;
                  if (current_data.size() > 1 && current_data.front() == '{')
                  {
                    // Insert "event":"<type>" into the data object
                    event_json = "{\"event\":\"" + current_event + "\"," + current_data.substr(1);
                  }
                  else
                  {
                    event_json = "{\"event\":\"" + current_event + "\",\"data\":" + current_data + "}";
                  }

                  std::lock_guard<std::mutex> lock(g_mutex);
                  g_event_queue.push_back({ target->id, std::move(event_json) });
                }

                if (current_event == "cancel" || current_event == "auth_revoked")
                {
                  Log::warning("[Firebase Listener] Watch {} received '{}', stopping",
                    target->id, current_event);
                  HTTP::close_stream(stream);
                  return; // Don't reconnect for permission errors
                }

                current_event.clear();
                current_data.clear();
              }
              else if (line.rfind("event:", 0) == 0)
              {
                // "event: put" or "event:put"
                size_t start = 6;
                if (start < line.size() && line[start] == ' ') ++start;
                current_event = line.substr(start);
              }
              else if (line.rfind("data:", 0) == 0)
              {
                // "data: {...}" or "data:{...}"
                size_t start = 5;
                if (start < line.size() && line[start] == ' ') ++start;
                if (current_data.empty())
                  current_data = line.substr(start);
                else
                  current_data += line.substr(start); // Multi-line data (rare)
              }
              // Ignore comment lines (starting with ':') and unknown fields
            }

            // Keep unprocessed remainder
            line_buffer.erase(0, pos);
          }

          HTTP::close_stream(stream);

          // If not cancelled, reconnect
          if (!target->cancel.load())
          {
            Log::info("[Firebase Listener] Watch {} disconnected, reconnecting in {}ms",
              target->id, backoff_ms);
            for (int slept = 0; slept < backoff_ms && !target->cancel.load(); slept += 100)
              std::this_thread::sleep_for(std::chrono::milliseconds(100));
            backoff_ms = (std::min)(backoff_ms * 2, MAX_BACKOFF_MS);
          }
        }

        Log::info("[Firebase Listener] Watch {} thread exiting", target->id);
      }

      // ================================================================
      // Public API
      // ================================================================

      int start_watch(const std::string& url, Callback on_change)
      {
        auto target = std::make_unique<WatchTarget>();
        target->id = g_next_id++;
        target->url = url;
        target->on_change = std::move(on_change);

        int id = target->id;
        WatchTarget* raw = target.get();
        target->thread = std::thread(reader_thread, raw);

        std::lock_guard<std::mutex> lock(g_mutex);
        g_watches.push_back(std::move(target));
        return id;
      }

      bool stop_watch(int watch_id)
      {
        std::unique_ptr<WatchTarget> found;
        {
          std::lock_guard<std::mutex> lock(g_mutex);
          for (auto it = g_watches.begin(); it != g_watches.end(); ++it)
          {
            if ((*it)->id == watch_id)
            {
              (*it)->cancel.store(true);
              found = std::move(*it);
              g_watches.erase(it);
              break;
            }
          }
        }

        if (found)
        {
          if (found->thread.joinable())
            found->thread.join();
          Log::info("[Firebase Listener] Watch {} stopped", watch_id);
          return true;
        }
        return false;
      }

      void stop_all()
      {
        std::vector<std::unique_ptr<WatchTarget>> all;
        {
          std::lock_guard<std::mutex> lock(g_mutex);
          all = std::move(g_watches);
          g_watches.clear();
        }

        for (auto& w : all)
        {
          w->cancel.store(true);
        }
        for (auto& w : all)
        {
          if (w->thread.joinable())
            w->thread.join();
        }

        Log::info("[Firebase Listener] All watches stopped");
      }

      void dispatch_events()
      {
        std::vector<SSEEvent> events;
        {
          std::lock_guard<std::mutex> lock(g_mutex);
          events.swap(g_event_queue);
        }

        for (const auto& evt : events)
        {
          // Find the callback for this watch_id
          Callback cb;
          {
            std::lock_guard<std::mutex> lock(g_mutex);
            for (const auto& w : g_watches)
            {
              if (w->id == evt.watch_id)
              {
                cb = w->on_change;
                break;
              }
            }
          }

          if (cb)
          {
            cb(true, evt.json);
          }
        }
      }

      size_t active_count()
      {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_watches.size();
      }

    } // namespace Listener
  } // namespace Firebase
} // namespace Engine
