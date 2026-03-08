#pragma once

#include "core/Experiment.hpp"
#include "benchmark/ComparativeBenchmark.hpp"
#include <iostream>
#include <memory>

namespace nomos {
namespace benchmark {

class ComparativeBenchmarkExperiment : public core::Experiment {
public:
    ComparativeBenchmarkExperiment()
        : m_benchmark(new ComparativeBenchmark()) {}

    int setup() override {
        std::cout << "[ComparativeBenchmark] Setting up..." << std::endl;
        return 0;
    }

    void run() override {
        std::cout << "[ComparativeBenchmark] Running comparative benchmarks..." << std::endl;

        std::vector<ComparisonResult> results;

        // Experiment 1: Varying N (number of files)
        std::cout << "\n=== Experiment 1: Scalability (varying N) ===" << std::endl;
        std::vector<size_t> N_values = {100, 500, 1000, 5000};
        for (size_t N : N_values) {
            BenchmarkConfig config;
            config.num_files = N;
            config.num_keywords = 100;
            config.num_updates = 100;
            config.num_searches = 10;
            config.cross_tags_l = 3;
            config.cross_tags_k = 2;

            std::cout << "\nN = " << N << std::endl;
            auto result = m_benchmark->runComparison(config);
            results.push_back(result);

            std::cout << "  Nomos: Update=" << result.nomos_result.avg_update_time_ms
                      << "ms, Search=" << result.nomos_result.avg_search_time_ms << "ms" << std::endl;
            std::cout << "  MC-ODXT: Update=" << result.mcodxt_result.avg_update_time_ms
                      << "ms, Search=" << result.mcodxt_result.avg_search_time_ms << "ms" << std::endl;
            std::cout << "  VQNomos: Update=" << result.vqnomos_result.avg_update_time_ms
                      << "ms, Search=" << result.vqnomos_result.avg_search_time_ms << "ms" << std::endl;
        }

        // Export results
        ComparativeBenchmark::exportToCSV(results, "comparative_results.csv");

        std::cout << "\n[ComparativeBenchmark] Benchmark complete!" << std::endl;
    }

    void teardown() override {
        std::cout << "[ComparativeBenchmark] Tearing down..." << std::endl;
    }

    std::string getName() const override {
        return "comparative-benchmark";
    }

private:
    std::unique_ptr<ComparativeBenchmark> m_benchmark;
};

}  // namespace benchmark
}  // namespace nomos
