#pragma once

#include <iostream>

#include "core/Experiment.hpp"
#include "benchmark/NomosBenchmark.hpp"

extern "C" {
#include <relic/relic.h>
}

namespace nomos {
namespace benchmark {

/**
 * @brief Benchmark experiment for Nomos performance evaluation
 *
 * Runs parameterized benchmarks and exports results to CSV/JSON
 * (Paper Section 6: Performance Evaluation)
 */
class BenchmarkExperiment : public core::Experiment {
public:
    BenchmarkExperiment() : benchmark_() {}
    ~BenchmarkExperiment() override = default;

    int setup() override {
        std::cout << "[Benchmark] Setting up..." << std::endl;

        // Initialize RELIC
        if (core_init() != RLC_OK) {
            std::cerr << "Failed to initialize RELIC" << std::endl;
            return -1;
        }

        // Set elliptic curve
        if (ep_param_set_any_pairf() != RLC_OK) {
            std::cerr << "Failed to set elliptic curve" << std::endl;
            return -1;
        }

        std::cout << "[Benchmark] Setup complete" << std::endl;
        return 0;
    }

    void run() override;

    void teardown() override {
        std::cout << "[Benchmark] Teardown complete" << std::endl;
        core_clean();
    }

    std::string getName() const override {
        return "Benchmark";
    }

private:
    NomosBenchmark benchmark_;

    /**
     * @brief Run a single benchmark configuration
     */
    void runSingleBenchmark(const BenchmarkConfig& config);

    /**
     * @brief Run parameter sweep benchmarks
     */
    void runParameterSweep();
};

}  // namespace benchmark
}  // namespace nomos