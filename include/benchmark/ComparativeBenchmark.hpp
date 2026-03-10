#ifndef NOMOS_COMPARATIVE_BENCHMARK_HPP
#define NOMOS_COMPARATIVE_BENCHMARK_HPP

#include "benchmark/BenchmarkFramework.hpp"
#include "benchmark/DatasetLoader.hpp"
#include <string>
#include <vector>

namespace nomos {
namespace benchmark {

/**
 * @brief Comparison result for three schemes
 *
 * Paper: Chapter 4 - Performance Evaluation
 * Compares Nomos baseline, MC-ODXT, and VQNomos
 */
struct ComparisonResult {
    std::string scheme_name;
    BenchmarkResult nomos_result;
    BenchmarkResult mcodxt_result;
    BenchmarkResult vqnomos_result;

    ComparisonResult() : scheme_name("comparison") {}
};

/**
 * @brief Comparative benchmark for three schemes
 *
 * Runs performance comparison between:
 * 1. Nomos - Multi-user multi-keyword baseline
 * 2. MC-ODXT - Multi-client ODXT adaptation
 * 3. VQNomos - Verifiable Nomos with QTree
 */
class ComparativeBenchmark {
public:
    ComparativeBenchmark();
    ~ComparativeBenchmark();

    /**
     * @brief Run comparison with given configuration
     * @param config Benchmark parameters
     * @return Comparison results for all three schemes
     */
    ComparisonResult runComparison(const BenchmarkConfig& config);

    /**
     * @brief Export comparison results to CSV
     * @param results Vector of comparison results
     * @param filename Output CSV file path
     */
    static void exportToCSV(const std::vector<ComparisonResult>& results,
                           const std::string& filename);

private:
    DatasetLoader dataset_loader_;

    // Helper: Run Nomos baseline benchmark
    BenchmarkResult runNomosBenchmark(const BenchmarkConfig& config);

    // Helper: Run MC-ODXT benchmark
    BenchmarkResult runMcOdxtBenchmark(const BenchmarkConfig& config);

    // Helper: Run VQNomos benchmark
    BenchmarkResult runVQNomosBenchmark(const BenchmarkConfig& config);
};

}  // namespace benchmark
}  // namespace nomos

#endif  // NOMOS_COMPARATIVE_BENCHMARK_HPP
