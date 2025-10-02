#include "TypeDef.h"
#include "Perf.h"
#include <iostream>

namespace Engine
{
  void Perf::start(const std::string& label)
  {
    start_time_map[label] = std::chrono::high_resolution_clock::now();
  }

  void Perf::stop(const std::string& label)
  {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      end_time - start_time_map[label]).count();
    elapsed_time_map[label] = duration / 1000000.0; // Convert to seconds
  }

  f32 Perf::get_elapsed(const std::string& label) const
  {
    auto it = elapsed_time_map.find(label);
    f32 elapsed = (it != elapsed_time_map.end()) ? static_cast<f32>(it->second) : 0.0f;
    std::cout << label << ": " << elapsed << " seconds (" << (elapsed * 1000.0) << " ms)" << std::endl;
    return elapsed;
  }

  const std::unordered_map<std::string, f64>& Perf::get_elapsed_time_map() const
  {
    return elapsed_time_map;
  }

  const f64 Perf::get_total_time() const
  {
    f64 total = 0.0;
    for (const auto& pair : elapsed_time_map)
    {
      total += pair.second;
    }
    return total;
  }
}