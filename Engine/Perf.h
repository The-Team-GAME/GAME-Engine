#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include <iostream>

namespace Engine
{
  class Perf
  {
  private:
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> start_time_map;
    std::unordered_map<std::string, f64> elapsed_time_map;

  public:
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

    f32 get_elapsed(const std::string& label) const
    {
      auto it = elapsed_time_map.find(label);
      f32 elapsed = (it != elapsed_time_map.end()) ? it->second : 0.0;
      std::cout << label << ": " << elapsed << " seconds (" << (elapsed * 1000.0) << " ms)" << std::endl;
      return elapsed;
    }

    const auto& get_elapsed_time_map() const
    {
      return elapsed_time_map;
    }

    const f64 get_total_time() const
    {
      f64 total = 0.0;
      for (const auto& pair : elapsed_time_map)
      {
        total += pair.second;
      }
      return total;
    }

  };
}
