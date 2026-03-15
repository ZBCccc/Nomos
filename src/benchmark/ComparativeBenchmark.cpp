#include "benchmark/ComparativeBenchmark.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>

#include "benchmark/NomosBenchmark.hpp"
#include "mc-odxt/McOdxtTypes.hpp"
#include "verifiable/QTree.hpp"

namespace nomos {
namespace benchmark {

ComparativeBenchmark::ComparativeBenchmark()
    : dataset_loader_(DatasetLoader::Dataset::None) {}

ComparativeBenchmark::~ComparativeBenchmark() {}

ComparisonResult ComparativeBenchmark::runComparison(
    const BenchmarkConfig& config) {
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

BenchmarkResult ComparativeBenchmark::runNomosBenchmark(
    const BenchmarkConfig& config) {
  NomosBenchmark nomos_bench;
  return nomos_bench.runBenchmark(config);
}

BenchmarkResult ComparativeBenchmark::runMcOdxtBenchmark(
    const BenchmarkConfig& config) {
  BenchmarkResult result;
  result.config = config;

  // Generate keywords using dataset loader
  std::vector<std::string> keywords =
      dataset_loader_.generateKeywords(config.num_keywords, 42);
  std::vector<std::string> file_ids =
      dataset_loader_.generateFileIds(config.num_files, 123);

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

  mcodxt::McOdxtServer server;
  server.setup(gatekeeper.getKm());
  mcodxt::McOdxtClient client;
  client.setup();

  auto end = std::chrono::high_resolution_clock::now();
  result.setup_time_ms =
      std::chrono::duration<double, std::milli>(end - start).count();

  // Update phase
  start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < config.num_updates; ++i) {
    const std::string& keyword = keywords[i % keywords.size()];
    const std::string& file_id = file_ids[i % file_ids.size()];
    auto update_meta = gatekeeper.update(mcodxt::OpType::ADD, file_id, keyword);
    server.update(update_meta);
  }
  end = std::chrono::high_resolution_clock::now();
  result.total_update_time_ms =
      std::chrono::duration<double, std::milli>(end - start).count();
  result.avg_update_time_ms = result.total_update_time_ms / config.num_updates;

  // Search phase - use keywords that were inserted
  start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < config.num_searches; ++i) {
    const std::string& keyword = search_keywords[i];
    std::vector<std::string> query = {keyword};

    auto token = client.genTokenSimplified(query, gatekeeper);

    auto search_req =
        client.prepareSearch(token, query, gatekeeper.getUpdateCounts());

    auto encrypted_results = server.search(search_req);

    client.decryptResults(encrypted_results, token);
  }
  end = std::chrono::high_resolution_clock::now();
  result.total_search_time_ms =
      std::chrono::duration<double, std::milli>(end - start).count();
  result.avg_search_time_ms = result.total_search_time_ms / config.num_searches;

  // Storage measurement
  const size_t TSET_ENTRY_SIZE = 113;
  const size_t XSET_ENTRY_SIZE = 33;
  result.tset_size_bytes = server.getTSetSize() * TSET_ENTRY_SIZE;
  result.xset_size_bytes = server.getXSetSize() * XSET_ENTRY_SIZE;
  result.total_storage_bytes = result.tset_size_bytes + result.xset_size_bytes;

  // Communication measurement
  const size_t EP_T_SIZE = 33;
  result.token_size_bytes =
      EP_T_SIZE + config.cross_tags_k * config.cross_tags_l * EP_T_SIZE;

  return result;
}

BenchmarkResult ComparativeBenchmark::runVQNomosBenchmark(
    const BenchmarkConfig& config) {
  // Paper: VQNomos = Nomos + QTree verifiability overhead (Section 5)
  // Runs the full Nomos protocol with actual QTree
  // updateBit/generatePositiveProof calls.
  BenchmarkResult result;
  result.config = config;

  std::vector<std::string> keywords =
      dataset_loader_.generateKeywords(config.num_keywords, 42);
  std::vector<std::string> file_ids =
      dataset_loader_.generateFileIds(config.num_files, 123);
  std::vector<std::string> search_keywords;
  for (size_t i = 0; i < config.num_searches; ++i) {
    search_keywords.push_back(keywords[i % config.num_updates]);
  }

  auto start = std::chrono::high_resolution_clock::now();
  nomos::Gatekeeper gatekeeper;
  gatekeeper.setup(10);
  nomos::Server server;
  nomos::Client client;
  client.setup();

  // QTree capacity = 1024 (default M from paper)
  const size_t qtree_capacity = 1024;
  verifiable::QTree qtree(qtree_capacity);
  std::vector<bool> initial_bits(qtree_capacity, false);
  qtree.initialize(initial_bits);

  auto end = std::chrono::high_resolution_clock::now();
  result.setup_time_ms =
      std::chrono::duration<double, std::milli>(end - start).count();

  start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < config.num_updates; ++i) {
    const std::string& keyword = keywords[i % keywords.size()];
    const std::string& file_id = file_ids[i % file_ids.size()];

    auto update_meta = gatekeeper.update(nomos::OP_ADD, file_id, keyword);
    server.update(update_meta);

    // Paper: VQNomos Section 5 - update XSet bit array in QTree
    for (const auto& xtag : update_meta.xtags) {
      qtree.updateBit(xtag, true);
    }
  }
  end = std::chrono::high_resolution_clock::now();
  result.total_update_time_ms =
      std::chrono::duration<double, std::milli>(end - start).count();
  result.avg_update_time_ms = result.total_update_time_ms / config.num_updates;

  start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < config.num_searches; ++i) {
    const std::string& keyword = search_keywords[i];
    std::vector<std::string> query = {keyword};

    auto search_token = client.genTokenSimplified(query, gatekeeper);
    auto search_req =
        client.prepareSearch(search_token, query, gatekeeper.getUpdateCounts());
    auto encrypted_results = server.search(search_req);
    auto ids = client.decryptResults(encrypted_results, search_token);

    // Paper: VQNomos Section 5 - generate positive proof for matched addresses
    if (!ids.empty()) {
      qtree.generatePositiveProof(ids);
    }
  }
  end = std::chrono::high_resolution_clock::now();
  result.total_search_time_ms =
      std::chrono::duration<double, std::milli>(end - start).count();
  result.avg_search_time_ms = result.total_search_time_ms / config.num_searches;

  const size_t TSET_ENTRY_SIZE = 113;
  const size_t XSET_ENTRY_SIZE = 33;
  result.tset_size_bytes = server.getTSetSize() * TSET_ENTRY_SIZE;
  result.xset_size_bytes = server.getXSetSize() * XSET_ENTRY_SIZE;
  const size_t qtree_storage = qtree_capacity * 32;
  result.total_storage_bytes =
      result.tset_size_bytes + result.xset_size_bytes + qtree_storage;

  const size_t EP_T_SIZE = 33;
  result.token_size_bytes =
      EP_T_SIZE + config.cross_tags_k * config.cross_tags_l * EP_T_SIZE;

  return result;
}

void ComparativeBenchmark::exportToCSV(
    const std::vector<ComparisonResult>& results, const std::string& filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
  }

  // CSV header
  file << "Scheme,N,Keywords,QuerySize,SetupTime,UpdateTime,SearchTime,"
          "Storage\n";

  for (const auto& comp : results) {
    // Nomos row
    const auto& nr = comp.nomos_result;
    file << "Nomos," << nr.config.num_files << "," << nr.config.num_keywords
         << "," << nr.config.result_set_size << "," << nr.setup_time_ms << ","
         << nr.avg_update_time_ms << "," << nr.avg_search_time_ms << ","
         << nr.total_storage_bytes << "\n";

    // MC-ODXT row
    const auto& mr = comp.mcodxt_result;
    file << "MC-ODXT," << mr.config.num_files << "," << mr.config.num_keywords
         << "," << mr.config.result_set_size << "," << mr.setup_time_ms << ","
         << mr.avg_update_time_ms << "," << mr.avg_search_time_ms << ","
         << mr.total_storage_bytes << "\n";

    // VQNomos row
    const auto& vr = comp.vqnomos_result;
    file << "VQNomos," << vr.config.num_files << "," << vr.config.num_keywords
         << "," << vr.config.result_set_size << "," << vr.setup_time_ms << ","
         << vr.avg_update_time_ms << "," << vr.avg_search_time_ms << ","
         << vr.total_storage_bytes << "\n";
  }

  file.close();
  std::cout << "[ComparativeBenchmark] Results exported to " << filename
            << std::endl;
}

}  // namespace benchmark
}  // namespace nomos
