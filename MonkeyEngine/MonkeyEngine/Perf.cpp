//-----------------------------------------------------------------------------------
// file:    Perf.cpp
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements a performance profiling utility that measures and
//          tracks execution time for labeled code sections using high-resolution
//          timers.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "TypeDef.h"
#include "Perf.h"
#include "Logger.h"

namespace Engine
{
  namespace Perf
  {
    namespace
    {
      std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> start_time_map;
      std::unordered_map<std::string, f64> elapsed_time_map;
    }

    /* ================ //
    // Member Functions //
    // ================ */
    void start(const std::string& label)
    {
      start_time_map[label] = std::chrono::high_resolution_clock::now();
    }

    void stop(const std::string& label)
    {
      auto end_time = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time_map[label]).count();
      elapsed_time_map[label] = duration / 1000000.0; // Convert to seconds
    }

    f32 get_elapsed(const std::string& label)
    {
      auto it = elapsed_time_map.find(label);
      f32 elapsed = (it != elapsed_time_map.end()) ? static_cast<f32>(it->second) : 0.0f;
      Log::info("{}: {} seconds ({} ms)", label, elapsed, elapsed * 1000.0);
      return elapsed;
    }

    const std::unordered_map<std::string, f64>& get_elapsed_time_map()
    {
      return elapsed_time_map;
    }

    f64 get_total_time()
    {
      f64 total = 0.0;
      for (const auto& pair : elapsed_time_map)
      {
        total += pair.second;
      }
      return total;
    }
  }
}