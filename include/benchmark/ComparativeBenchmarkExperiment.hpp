#pragma once

#include "core/Experiment.hpp"
#include "benchmark/ComparativeBenchmark.hpp"
#include "benchmark/DatasetLoader.hpp"
#include <iostream>
#include <memory>

namespace nomos {
namespace benchmark {

class ComparativeBenchmarkExperiment : public core::Experiment {
public:
    ComparativeBenchmarkExperiment()
        : m_benchmark(new ComparativeBenchmark()),
          m_dataset(DatasetLoader::Dataset::None),
          m_num_files(1000),
          m_num_keywords(100),
          m_num_updates(100),
          m_num_searches(10) {}

    int setup() override {
        std::cout << "[ComparativeBenchmark] Setting up..." << std::endl;
        return 0;
    }

    void run() override {
        std::cout << "[ComparativeBenchmark] Running comparative benchmarks..." << std::endl;
        std::cout << "Dataset: " << datasetToString(m_dataset) << std::endl;
        std::cout << "N (files): " << m_num_files << std::endl;
        std::cout << "Keywords: " << m_num_keywords << std::endl;

        std::vector<ComparisonResult> results;

        // Experiment: Scalability test with fixed N or varying N
        std::cout << "\n=== Scalability Test ===" << std::endl;

        std::vector<size_t> N_values;
        if (m_scalability_test) {
            N_values = {100, 500, 1000, 5000};
        } else {
            N_values = {m_num_files};
        }

        for (size_t N : N_values) {
            BenchmarkConfig config;
            config.num_files = N;
            config.num_keywords = m_num_keywords;
            config.num_updates = std::min(m_num_updates, N);
            config.num_searches = m_num_searches;
            config.cross_tags_l = 3;
            config.cross_tags_k = 2;
            config.dataset = m_dataset;

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
        std::string filename = "comparative_results_" + datasetToString(m_dataset) + ".csv";
        ComparativeBenchmark::exportToCSV(results, filename);

        std::cout << "\n[ComparativeBenchmark] Benchmark complete!" << std::endl;
    }

    void teardown() override {
        std::cout << "[ComparativeBenchmark] Tearing down..." << std::endl;
    }

    std::string getName() const override {
        return "comparative-benchmark";
    }

    void setDataset(DatasetLoader::Dataset dataset) { m_dataset = dataset; }
    void setScalabilityTest(bool value) { m_scalability_test = value; }
    void setNumFiles(size_t n) { m_num_files = n; }
    void setNumKeywords(size_t n) { m_num_keywords = n; }
    void setNumUpdates(size_t n) { m_num_updates = n; }
    void setNumSearches(size_t n) { m_num_searches = n; }

private:
    std::unique_ptr<ComparativeBenchmark> m_benchmark;
    DatasetLoader::Dataset m_dataset;
    size_t m_num_files;
    size_t m_num_keywords;
    size_t m_num_updates;
    size_t m_num_searches;
    bool m_scalability_test = false;
};

}  // namespace benchmark
}  // namespace nomos
