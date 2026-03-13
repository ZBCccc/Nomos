#ifndef NOMOS_NOMOS_BENCHMARK_HPP
#define NOMOS_NOMOS_BENCHMARK_HPP

#include "benchmark/BenchmarkFramework.hpp"
#include "benchmark/DatasetLoader.hpp"
#include "nomos/Gatekeeper.hpp"
#include "nomos/Server.hpp"
#include "nomos/Client.hpp"
#include <memory>

namespace nomos {
namespace benchmark {

/**
 * @brief Benchmark implementation for Nomos scheme
 *
 * Measures performance of the Nomos simplified scheme
 * (Paper: Bag et al. 2024, Section 6: Performance Evaluation)
 *
 * Uses the main Nomos implementation:
 * - Gatekeeper: Token generation
 * - Server: Encrypted index storage
 * - Client: Search operations
 */
class NomosBenchmark : public BenchmarkFramework {
public:
    NomosBenchmark();
    ~NomosBenchmark();

    /**
     * @brief Run Nomos benchmark with given configuration
     *
     * Workflow:
     * 1. Setup: Initialize Gatekeeper, Server, Client
     * 2. Update: Perform num_updates insertions
     * 3. Search: Perform num_searches queries
     * 4. Measure: Collect timing and storage metrics
     *
     * @param config Benchmark parameters
     * @return Benchmark results
     */
    BenchmarkResult runBenchmark(const BenchmarkConfig& config) override;

private:
    std::unique_ptr<nomos::Gatekeeper> gatekeeper_;
    std::unique_ptr<nomos::Server> server_;
    std::unique_ptr<nomos::Client> client_;
    DatasetLoader dataset_loader_;

    /**
     * @brief Setup phase: Initialize all components
     * @param config Benchmark configuration
     * @return Setup time in milliseconds
     */
    double setupPhase(const BenchmarkConfig& config);

    /**
     * @brief Update phase: Perform insertions
     * @param config Benchmark configuration
     * @return Total update time in milliseconds
     */
    double updatePhase(const BenchmarkConfig& config);

    /**
     * @brief Search phase: Perform queries
     * @param config Benchmark configuration
     * @return Total search time in milliseconds
     */
    double searchPhase(const BenchmarkConfig& config);

    /**
     * @brief Measure storage overhead
     * @param result Output: storage metrics
     */
    void measureStorage(BenchmarkResult& result);

    /**
     * @brief Measure communication overhead
     * @param result Output: communication metrics
     */
    void measureCommunication(BenchmarkResult& result);
};

}  // namespace benchmark
}  // namespace nomos

#endif  // NOMOS_NOMOS_BENCHMARK_HPP
