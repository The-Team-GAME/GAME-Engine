#pragma once
#include <string>
#include <unordered_map>
#include <chrono>

namespace Engine
{
  class Perf
  {
  private:
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> start_time_map;
    std::unordered_map<std::string, f64> elapsed_time_map;

  public:
    void start(const std::string& label);
    void stop(const std::string& label);
    f32 get_elapsed(const std::string& label) const;
    const std::unordered_map<std::string, f64>& get_elapsed_time_map() const;
    const f64 get_total_time() const;
  };
}
