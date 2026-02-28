#ifndef NOMOS_BENCHMARK_FRAMEWORK_HPP
#define NOMOS_BENCHMARK_FRAMEWORK_HPP

#include <chrono>
#include <string>
#include <vector>

namespace nomos {
namespace benchmark {

/**
 * @brief Configuration for benchmark experiments
 *
 * Allows parameter scanning for experimental evaluation
 * (Paper Section 6: Performance Evaluation)
 */
struct BenchmarkConfig {
    size_t num_keywords;      // N: number of keywords
    size_t num_files;         // M: number of files
    size_t cross_tags_l;      // ℓ: cross-tags per insertion (default: 3)
    size_t cross_tags_k;      // k: cross-tags sampled during query (default: 2)
    size_t result_set_size;   // n: expected search result size
    size_t num_updates;       // Number of update operations to perform
    size_t num_searches;      // Number of search operations to perform

    BenchmarkConfig()
        : num_keywords(100),
          num_files(1000),
          cross_tags_l(3),
          cross_tags_k(2),
          result_set_size(10),
          num_updates(100),
          num_searches(10) {}
};

/**
 * @brief Results from a benchmark run
 *
 * Captures all metrics needed for thesis Chapter 4
 * (Paper Section 6: Experimental Results)
 */
struct BenchmarkResult {
    // Timing measurements (milliseconds)
    double setup_time_ms;
    double total_update_time_ms;
    double avg_update_time_ms;
    double total_search_time_ms;
    double avg_search_time_ms;

    // Storage overhead (bytes)
    size_t tset_size_bytes;
    size_t xset_size_bytes;
    size_t total_storage_bytes;

    // Communication overhead (bytes)
    size_t token_size_bytes;

    // Configuration used
    BenchmarkConfig config;

    BenchmarkResult()
        : setup_time_ms(0.0),
          total_update_time_ms(0.0),
          avg_update_time_ms(0.0),
          total_search_time_ms(0.0),
          avg_search_time_ms(0.0),
          tset_size_bytes(0),
          xset_size_bytes(0),
          total_storage_bytes(0),
          token_size_bytes(0) {}
};

/**
 * @brief Base class for benchmark frameworks
 *
 * Provides timing utilities and data export functionality
 * for searchable encryption scheme evaluation
 */
class BenchmarkFramework {
public:
    virtual ~BenchmarkFramework() {}

    /**
     * @brief Run benchmark with given configuration
     * @param config Benchmark parameters
     * @return Benchmark results with timing and storage metrics
     */
    virtual BenchmarkResult runBenchmark(const BenchmarkConfig& config) = 0;

    /**
     * @brief Export results to CSV format
     * @param results Vector of benchmark results
     * @param filename Output CSV file path
     */
    static void exportToCSV(const std::vector<BenchmarkResult>& results,
                           const std::string& filename);

    /**
     * @brief Export results to JSON format
     * @param results Vector of benchmark results
     * @param filename Output JSON file path
     */
    static void exportToJSON(const std::vector<BenchmarkResult>& results,
                            const std::string& filename);

protected:
    /**
     * @brief High-resolution timer for accurate measurements
     */
    class Timer {
    public:
        Timer() : start_(std::chrono::high_resolution_clock::now()) {}

        void reset() {
            start_ = std::chrono::high_resolution_clock::now();
        }

        double elapsedMilliseconds() const {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end - start_);
            return duration.count() / 1000.0;
        }

    private:
        std::chrono::high_resolution_clock::time_point start_;
    };
};

}  // namespace benchmark
}  // namespace nomos

#endif  // NOMOS_BENCHMARK_FRAMEWORK_HPP
