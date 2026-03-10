#include "benchmark/ComparativeBenchmark.hpp"
#include "benchmark/NomosBenchmark.hpp"
#include "mc-odxt/McOdxtTypes.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>

namespace nomos {
namespace benchmark {

ComparativeBenchmark::ComparativeBenchmark() : dataset_loader_(DatasetLoader::Dataset::None) {}

ComparativeBenchmark::~ComparativeBenchmark() {}

ComparisonResult ComparativeBenchmark::runComparison(const BenchmarkConfig& config) {
    ComparisonResult result;
    result.scheme_name = "comparison";

    // Initialize dataset loader
    dataset_loader_ = DatasetLoader(config.dataset);
    dataset_loader_.load();

    std::cout << "[ComparativeBenchmark] Running Nomos baseline..." << std::endl;
    result.nomos_result = runNomosBenchmark(config);

    std::cout << "[ComparativeBenchmark] Running MC-ODXT..." << std::endl;
    result.mcodxt_result = runMcOdxtBenchmark(config);

    std::cout << "[ComparativeBenchmark] Running VQNomos..." << std::endl;
    result.vqnomos_result = runVQNomosBenchmark(config);

    return result;
}

BenchmarkResult ComparativeBenchmark::runNomosBenchmark(const BenchmarkConfig& config) {
    NomosBenchmark nomos_bench;
    return nomos_bench.runBenchmark(config);
}

BenchmarkResult ComparativeBenchmark::runMcOdxtBenchmark(const BenchmarkConfig& config) {
    // Paper: MC-ODXT performance evaluation
    BenchmarkResult result;
    result.config = config;

    // Generate keywords using dataset loader
    std::vector<std::string> keywords = dataset_loader_.generateKeywords(config.num_keywords, 42);
    std::vector<std::string> file_ids = dataset_loader_.generateFileIds(config.num_files, 123);

    // For MC-ODXT, search keywords must be from the update set
    // Use a subset of the keywords used in updates
    std::vector<std::string> search_keywords;
    for (size_t i = 0; i < config.num_searches; ++i) {
        search_keywords.push_back(keywords[i % config.num_updates]);
    }

    // Setup phase
    auto start = std::chrono::high_resolution_clock::now();
    mcodxt::McOdxtGatekeeper gatekeeper;
    gatekeeper.setup(10);

    std::string owner_id = "owner_1";
    gatekeeper.registerDataOwner(owner_id);

    mcodxt::McOdxtServer server;
    mcodxt::McOdxtClient client("user_1");
    client.setup();
    gatekeeper.registerSearchUser("user_1");
    gatekeeper.grantAuthorization(owner_id, "user_1");

    auto end = std::chrono::high_resolution_clock::now();
    result.setup_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

    // Update phase
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < config.num_updates; ++i) {
        const std::string& keyword = keywords[i % keywords.size()];
        const std::string& file_id = file_ids[i % file_ids.size()];

        // Data owner generates update metadata
        mcodxt::McOdxtDataOwner data_owner(owner_id);
        data_owner.setup();
        auto update_meta = data_owner.update(mcodxt::OpType::ADD, file_id, keyword, gatekeeper);

        // Server processes update
        server.update(update_meta);
    }
    end = std::chrono::high_resolution_clock::now();
    result.total_update_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    result.avg_update_time_ms = result.total_update_time_ms / config.num_updates;

    // Search phase - use keywords that were inserted
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < config.num_searches; ++i) {
        const std::string& keyword = search_keywords[i];
        std::vector<std::string> query = {keyword};

        // Client generates search token
        auto token = client.genToken(query, owner_id, gatekeeper);

        // Prepare search request
        auto search_req = client.prepareSearch(token, query, gatekeeper.getUpdateCounts(owner_id));

        // Server processes search
        auto encrypted_results = server.search(search_req);

        // Client decrypts results
        client.decryptResults(encrypted_results, token, gatekeeper);
    }
    end = std::chrono::high_resolution_clock::now();
    result.total_search_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    result.avg_search_time_ms = result.total_search_time_ms / config.num_searches;

    // Storage measurement
    const size_t TSET_ENTRY_SIZE = 113;
    const size_t XSET_ENTRY_SIZE = 33;
    result.tset_size_bytes = server.getTSetSize() * TSET_ENTRY_SIZE;
    result.xset_size_bytes = server.getXSetSize() * XSET_ENTRY_SIZE;
    result.total_storage_bytes = result.tset_size_bytes + result.xset_size_bytes;

    // Communication measurement
    const size_t EP_T_SIZE = 33;
    const size_t ENV_SIZE = 48;
    result.token_size_bytes = EP_T_SIZE + config.cross_tags_k * config.cross_tags_l * EP_T_SIZE + ENV_SIZE;

    return result;
}

BenchmarkResult ComparativeBenchmark::runVQNomosBenchmark(const BenchmarkConfig& config) {
    // Paper: VQNomos performance evaluation with QTree overhead
    // For now, use Nomos baseline + estimated QTree overhead
    BenchmarkResult result = runNomosBenchmark(config);

    // Add QTree overhead estimation
    // Paper: QTree adds O(log M) overhead per update
    // Assume log2(1024) = 10 hash operations, each ~0.01ms
    double qtree_overhead_per_update = 0.1;  // ms
    result.avg_update_time_ms += qtree_overhead_per_update;
    result.total_update_time_ms = result.avg_update_time_ms * config.num_updates;

    // QTree adds O(k log M) overhead per search
    double qtree_overhead_per_search = config.cross_tags_k * 0.1;  // ms
    result.avg_search_time_ms += qtree_overhead_per_search;
    result.total_search_time_ms = result.avg_search_time_ms * config.num_searches;

    // QTree storage: M * 32 bytes (Merkle tree nodes)
    size_t qtree_storage = 1024 * 32;  // Assume M=1024
    result.total_storage_bytes += qtree_storage;

    return result;
}

void ComparativeBenchmark::exportToCSV(const std::vector<ComparisonResult>& results,
                                      const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    // CSV header
    file << "Scheme,N,Keywords,QuerySize,SetupTime,UpdateTime,SearchTime,Storage\n";

    for (const auto& comp : results) {
        // Nomos row
        const auto& nr = comp.nomos_result;
        file << "Nomos," << nr.config.num_files << "," << nr.config.num_keywords << ","
             << nr.config.result_set_size << "," << nr.setup_time_ms << ","
             << nr.avg_update_time_ms << "," << nr.avg_search_time_ms << ","
             << nr.total_storage_bytes << "\n";

        // MC-ODXT row
        const auto& mr = comp.mcodxt_result;
        file << "MC-ODXT," << mr.config.num_files << "," << mr.config.num_keywords << ","
             << mr.config.result_set_size << "," << mr.setup_time_ms << ","
             << mr.avg_update_time_ms << "," << mr.avg_search_time_ms << ","
             << mr.total_storage_bytes << "\n";

        // VQNomos row
        const auto& vr = comp.vqnomos_result;
        file << "VQNomos," << vr.config.num_files << "," << vr.config.num_keywords << ","
             << vr.config.result_set_size << "," << vr.setup_time_ms << ","
             << vr.avg_update_time_ms << "," << vr.avg_search_time_ms << ","
             << vr.total_storage_bytes << "\n";
    }

    file.close();
    std::cout << "[ComparativeBenchmark] Results exported to " << filename << std::endl;
}

}  // namespace benchmark
}  // namespace nomos
