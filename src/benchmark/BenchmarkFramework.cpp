#include "benchmark/BenchmarkFramework.hpp"

#include <fstream>
#include <iomanip>

namespace nomos {
namespace benchmark {

void BenchmarkFramework::exportToCSV(
    const std::vector<BenchmarkResult>& results, const std::string& filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
  }

  // Write CSV header
  file << "num_keywords,num_files,cross_tags_l,cross_tags_k,"
       << "result_set_size,num_updates,num_searches,"
       << "setup_time_ms,total_update_time_ms,avg_update_time_ms,"
       << "total_search_time_ms,avg_search_time_ms,"
       << "tset_size_bytes,xset_size_bytes,total_storage_bytes,"
       << "token_size_bytes\n";

  // Write data rows
  for (const auto& result : results) {
    file << result.config.num_keywords << "," << result.config.num_files << ","
         << result.config.cross_tags_l << "," << result.config.cross_tags_k
         << "," << result.config.result_set_size << ","
         << result.config.num_updates << "," << result.config.num_searches
         << "," << std::fixed << std::setprecision(3) << result.setup_time_ms
         << "," << result.total_update_time_ms << ","
         << result.avg_update_time_ms << "," << result.total_search_time_ms
         << "," << result.avg_search_time_ms << "," << result.tset_size_bytes
         << "," << result.xset_size_bytes << "," << result.total_storage_bytes
         << "," << result.token_size_bytes << "\n";
  }

  file.close();
}

void BenchmarkFramework::exportToJSON(
    const std::vector<BenchmarkResult>& results, const std::string& filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
  }

  file << "{\n";
  file << "  \"benchmark_results\": [\n";

  for (size_t i = 0; i < results.size(); ++i) {
    const auto& result = results[i];
    file << "    {\n";
    file << "      \"config\": {\n";
    file << "        \"num_keywords\": " << result.config.num_keywords << ",\n";
    file << "        \"num_files\": " << result.config.num_files << ",\n";
    file << "        \"cross_tags_l\": " << result.config.cross_tags_l << ",\n";
    file << "        \"cross_tags_k\": " << result.config.cross_tags_k << ",\n";
    file << "        \"result_set_size\": " << result.config.result_set_size
         << ",\n";
    file << "        \"num_updates\": " << result.config.num_updates << ",\n";
    file << "        \"num_searches\": " << result.config.num_searches << "\n";
    file << "      },\n";
    file << "      \"timing\": {\n";
    file << "        \"setup_time_ms\": " << std::fixed << std::setprecision(3)
         << result.setup_time_ms << ",\n";
    file << "        \"total_update_time_ms\": " << result.total_update_time_ms
         << ",\n";
    file << "        \"avg_update_time_ms\": " << result.avg_update_time_ms
         << ",\n";
    file << "        \"total_search_time_ms\": " << result.total_search_time_ms
         << ",\n";
    file << "        \"avg_search_time_ms\": " << result.avg_search_time_ms
         << "\n";
    file << "      },\n";
    file << "      \"storage\": {\n";
    file << "        \"tset_size_bytes\": " << result.tset_size_bytes << ",\n";
    file << "        \"xset_size_bytes\": " << result.xset_size_bytes << ",\n";
    file << "        \"total_storage_bytes\": " << result.total_storage_bytes
         << "\n";
    file << "      },\n";
    file << "      \"communication\": {\n";
    file << "        \"token_size_bytes\": " << result.token_size_bytes << "\n";
    file << "      }\n";
    file << "    }";
    if (i < results.size() - 1) {
      file << ",";
    }
    file << "\n";
  }

  file << "  ]\n";
  file << "}\n";

  file.close();
}

}  // namespace benchmark
}  // namespace nomos
