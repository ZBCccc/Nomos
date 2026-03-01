#include "benchmark/BenchmarkExperiment.hpp"

#include <iostream>
#include <vector>

namespace nomos {
namespace benchmark {

void BenchmarkExperiment::run() {
  std::cout << "[Benchmark] Running Nomos performance benchmarks..."
            << std::endl;

  // 按计划运行：关键词10,100,1K,10K + 文档100,1K,10K,100K
  std::vector<BenchmarkConfig> configs;
  
  // 关键词数量测试
  std::vector<size_t> keyword_counts = {10, 100, 1000, 10000};
  std::vector<size_t> file_counts = {100, 1000, 10000, 100000};
  
  for (size_t kw : keyword_counts) {
    for (size_t fc : file_counts) {
      BenchmarkConfig config;
      config.num_keywords = kw;
      config.num_files = fc;
      config.cross_tags_l = 3;
      config.cross_tags_k = 2;
      config.result_set_size = 5;
      config.num_updates = std::min(fc, (size_t)100);  // 最多100次更新
      config.num_searches = 10;
      configs.push_back(config);
    }
  }

  std::vector<BenchmarkResult> all_results;
  
  for (size_t i = 0; i < configs.size(); ++i) {
    std::cout << "\n=== Config " << (i+1) << "/" << configs.size() << " ===" << std::endl;
    std::cout << "Keywords: " << configs[i].num_keywords 
              << ", Files: " << configs[i].num_files << std::endl;
    
    try {
      auto result = benchmark_.runBenchmark(configs[i]);
      result.config = configs[i];  // 保存配置
      all_results.push_back(result);
      
      std::cout << "Setup: " << result.setup_time_ms << " ms" << std::endl;
      std::cout << "Avg Update: " << result.avg_update_time_ms << " ms" << std::endl;
      std::cout << "Avg Search: " << result.avg_search_time_ms << " ms" << std::endl;
      std::cout << "Storage: " << result.total_storage_bytes << " bytes" << std::endl;
    } catch (const std::exception& e) {
      std::cerr << "[Benchmark] Error: " << e.what() << std::endl;
    }
  }

  // 导出所有结果
  try {
    BenchmarkFramework::exportToCSV(all_results, "benchmark_results.csv");
    BenchmarkFramework::exportToJSON(all_results, "benchmark_results.json");
    std::cout << "\nResults exported!" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "[Benchmark] Export error: " << e.what() << std::endl;
  }

  std::cout << "[Benchmark] All benchmarks complete!" << std::endl;
}

void BenchmarkExperiment::runSingleBenchmark(const BenchmarkConfig& config) {
  std::cout << "\n=== Benchmark Configuration ===" << std::endl;
  std::cout << "Keywords: " << config.num_keywords << std::endl;
  std::cout << "Files: " << config.num_files << std::endl;

  BenchmarkResult result;
  try {
    result = benchmark_.runBenchmark(config);
  } catch (const std::exception& e) {
    std::cerr << "[Benchmark] Error: " << e.what() << std::endl;
    throw;
  }

  std::cout << "\n=== Results ===" << std::endl;
  std::cout << "Setup: " << result.setup_time_ms << " ms" << std::endl;
  std::cout << "Avg Update: " << result.avg_update_time_ms << " ms" << std::endl;
  std::cout << "Avg Search: " << result.avg_search_time_ms << " ms" << std::endl;
  std::cout << "Storage: " << result.total_storage_bytes << " bytes" << std::endl;

  std::vector<BenchmarkResult> results = {result};
  BenchmarkFramework::exportToCSV(results, "benchmark_results.csv");
  BenchmarkFramework::exportToJSON(results, "benchmark_results.json");
}

void BenchmarkExperiment::runParameterSweep() {
  // Implemented in run()
}

}  // namespace benchmark
}  // namespace nomos
