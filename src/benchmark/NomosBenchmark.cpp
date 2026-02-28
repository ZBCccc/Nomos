#include "benchmark/NomosBenchmark.hpp"

#include <string>
#include <vector>

namespace nomos {
namespace benchmark {

NomosBenchmark::NomosBenchmark()
    : gatekeeper_(nullptr), server_(nullptr), client_(nullptr) {}

NomosBenchmark::~NomosBenchmark() {
  // RELIC resource cleanup:
  // - GatekeeperCorrect destructor frees bn_t keys (m_Ks, m_Ky, m_Kt[], m_Kx[])
  // - ServerCorrect destructor clears m_TSet (TSetEntry destructor frees bn_t
  // alpha)
  // - ClientCorrect has no RELIC resources to clean up
  // - unique_ptr automatically calls destructors in reverse order
}

BenchmarkResult NomosBenchmark::runBenchmark(const BenchmarkConfig& config) {
  BenchmarkResult result;
  result.config = config;

  // Phase 1: Setup
  result.setup_time_ms = setupPhase(config);

  // Phase 2: Update
  result.total_update_time_ms = updatePhase(config);
  result.avg_update_time_ms = result.total_update_time_ms / config.num_updates;

  // Phase 3: Search
  result.total_search_time_ms = searchPhase(config);
  result.avg_search_time_ms = result.total_search_time_ms / config.num_searches;

  // Phase 4: Measure storage and communication
  measureStorage(result);
  measureCommunication(result);

  return result;
}

double NomosBenchmark::setupPhase(const BenchmarkConfig& config) {
  // Initialize components (not timed - object allocation overhead)
  try {
    gatekeeper_ = std::unique_ptr<nomos::GatekeeperCorrect>(
        new nomos::GatekeeperCorrect());
    server_ = std::unique_ptr<nomos::ServerCorrect>(new nomos::ServerCorrect());
    client_ = std::unique_ptr<nomos::ClientCorrect>(new nomos::ClientCorrect());
  } catch (const std::exception& e) {
    throw std::runtime_error("Failed to allocate benchmark components: " +
                             std::string(e.what()));
  }

  // Start timing for actual cryptographic setup
  Timer timer;

  // Setup with parameters
  // d = key array size (default 10 is fine for benchmarking)
  // Note: RELIC must be initialized before calling setup()
  int ret = gatekeeper_->setup(10);
  if (ret != 0) {
    throw std::runtime_error("Gatekeeper setup failed with code " +
                             std::to_string(ret));
  }

  ret = client_->setup();
  if (ret != 0) {
    throw std::runtime_error("Client setup failed with code " +
                             std::to_string(ret));
  }

  return timer.elapsedMilliseconds();
}

double NomosBenchmark::updatePhase(const BenchmarkConfig& config) {
  // Generate test data BEFORE starting timer (avoid measurement bias)
  std::vector<std::string> keywords;
  keywords.reserve(config.num_keywords);
  for (size_t i = 0; i < config.num_keywords; ++i) {
    keywords.push_back("keyword_" + std::to_string(i));
  }

  std::vector<std::string> file_ids;
  file_ids.reserve(config.num_files);
  for (size_t i = 0; i < config.num_files; ++i) {
    file_ids.push_back("file_" + std::to_string(i));
  }

  // Start timing for actual cryptographic operations
  Timer timer;

  // Perform updates (insertions)
  for (size_t i = 0; i < config.num_updates; ++i) {
    // Select keyword and file (round-robin for simplicity)
    const std::string& keyword = keywords[i % keywords.size()];
    const std::string& file_id = file_ids[i % file_ids.size()];

    // Generate update metadata from Gatekeeper (Algorithm 2)
    auto update_meta = gatekeeper_->update(OP_ADD, file_id, keyword);

    // Server processes update
    server_->update(update_meta);
  }

  return timer.elapsedMilliseconds();
}

double NomosBenchmark::searchPhase(const BenchmarkConfig& config) {
  // Generate test keywords BEFORE starting timer (avoid measurement bias)
  std::vector<std::string> search_keywords;
  search_keywords.reserve(config.num_searches);
  for (size_t i = 0; i < config.num_searches; ++i) {
    search_keywords.push_back("keyword_" +
                              std::to_string(i % config.num_keywords));
  }

  // Start timing for actual cryptographic operations
  Timer timer;

  // Perform searches
  for (const auto& keyword : search_keywords) {
    // Generate search token from Gatekeeper (simplified - Algorithm 3)
    std::vector<std::string> query = {keyword};
    auto search_token = client_->genTokenSimplified(query, *gatekeeper_);

    // Prepare search request (Algorithm 4 - Client side)
    auto search_req = client_->prepareSearch(search_token, query,
                                             gatekeeper_->getUpdateCounts());

    // Server processes search (Algorithm 4 - Server side)
    auto encrypted_results = server_->search(search_req);

    // Client decrypts results
    client_->decryptResults(encrypted_results, search_token);
  }

  return timer.elapsedMilliseconds();
}

void NomosBenchmark::measureStorage(BenchmarkResult& result) {
  // Accurate storage estimation based on RELIC types:
  // TSet entry: ep_t addr (33 bytes compressed) + encrypted(id||op) (~48 bytes
  // AES) + bn_t alpha (32 bytes) = 113 bytes XSet entry: ep_t xtag (33 bytes
  // compressed)
  const size_t TSET_ENTRY_SIZE = 113;  // ep_t(33) + AES(48) + bn_t(32)
  const size_t XSET_ENTRY_SIZE = 33;   // ep_t(33) compressed

  result.tset_size_bytes = server_->getTSetSize() * TSET_ENTRY_SIZE;
  result.xset_size_bytes = server_->getXSetSize() * XSET_ENTRY_SIZE;
  result.total_storage_bytes = result.tset_size_bytes + result.xset_size_bytes;
}

void NomosBenchmark::measureCommunication(BenchmarkResult& result) {
  // Accurate token size estimation based on RELIC types:
  // - stag: 1 ep_t = 33 bytes (compressed elliptic curve point)
  // - xtokens: k * ℓ * ep_t = k * ℓ * 33 bytes
  // - env: AES-128 encrypted data = 48 bytes (typical for id||op)
  const size_t EP_T_SIZE = 33;  // Compressed elliptic curve point
  const size_t ENV_SIZE = 48;   // AES-128 encrypted payload

  size_t stag_size = EP_T_SIZE;
  size_t xtokens_size =
      result.config.cross_tags_k * result.config.cross_tags_l * EP_T_SIZE;
  result.token_size_bytes = stag_size + xtokens_size + ENV_SIZE;
}

}  // namespace benchmark
}  // namespace nomos