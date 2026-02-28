#include "benchmark/BenchmarkExperiment.hpp"

#include <iostream>
#include <vector>

namespace nomos {
namespace benchmark {

void BenchmarkExperiment::run() {
  std::cout << "[Benchmark] Running Nomos performance benchmarks..."
            << std::endl;

  // Run default benchmark configuration
  BenchmarkConfig config;
  config.num_keywords = 10;
  config.num_files = 50;
  config.cross_tags_l = 3;
  config.cross_tags_k = 2;
  config.result_set_size = 5;
  config.num_updates = 20;
  config.num_searches = 5;

  try {
    runSingleBenchmark(config);
  } catch (const std::exception& e) {
    std::cerr << "[Benchmark] Error: " << e.what() << std::endl;
    throw;  // Re-throw to allow caller to handle
  }

  std::cout << "[Benchmark] All benchmarks complete!" << std::endl;
}

void BenchmarkExperiment::runSingleBenchmark(const BenchmarkConfig& config) {
  std::cout << "\n=== Benchmark Configuration ===" << std::endl;
  std::cout << "Keywords: " << config.num_keywords << std::endl;
  std::cout << "Files: " << config.num_files << std::endl;
  std::cout << "Cross-tags (ℓ): " << config.cross_tags_l << std::endl;
  std::cout << "Cross-tags sampled (k): " << config.cross_tags_k << std::endl;
  std::cout << "Updates: " << config.num_updates << std::endl;
  std::cout << "Searches: " << config.num_searches << std::endl;

  // Run benchmark with exception handling
  std::cout << "\nRunning benchmark..." << std::endl;
  BenchmarkResult result;

  try {
    result = benchmark_.runBenchmark(config);
  } catch (const std::exception& e) {
    std::cerr << "[Benchmark] Benchmark execution failed: " << e.what()
              << std::endl;
    throw;
  }

  // Display results
  std::cout << "\n=== Benchmark Results ===" << std::endl;
  std::cout << "Setup time: " << result.setup_time_ms << " ms" << std::endl;
  std::cout << "Total update time: " << result.total_update_time_ms << " ms"
            << std::endl;
  std::cout << "Avg update time: " << result.avg_update_time_ms << " ms"
            << std::endl;
  std::cout << "Total search time: " << result.total_search_time_ms << " ms"
            << std::endl;
  std::cout << "Avg search time: " << result.avg_search_time_ms << " ms"
            << std::endl;
  std::cout << "TSet size: " << result.tset_size_bytes << " bytes" << std::endl;
  std::cout << "XSet size: " << result.xset_size_bytes << " bytes" << std::endl;
  std::cout << "Total storage: " << result.total_storage_bytes << " bytes"
            << std::endl;
  std::cout << "Token size: " << result.token_size_bytes << " bytes"
            << std::endl;

  // Export results with exception handling
  try {
    std::vector<BenchmarkResult> results = {result};
    BenchmarkFramework::exportToCSV(results, "benchmark_results.csv");
    BenchmarkFramework::exportToJSON(results, "benchmark_results.json");

    std::cout << "\nResults exported to:" << std::endl;
    std::cout << "  - benchmark_results.csv" << std::endl;
    std::cout << "  - benchmark_results.json" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "[Benchmark] Warning: Failed to export results: " << e.what()
              << std::endl;
    // Don't re-throw - export failure shouldn't fail the entire benchmark
  }
}

void BenchmarkExperiment::runParameterSweep() {
  // TODO: Implement parameter sweep for thesis experiments
  // Sweep over: N (keywords), M (files), ℓ, k, n (result set size)
}

}  // namespace benchmark
}  // namespace nomos
