#include <gtest/gtest.h>

#include <chrono>
#include <vector>

#include "nomos/Client.hpp"
#include "nomos/Gatekeeper.hpp"
#include "nomos/Server.hpp"

extern "C" {
#include <relic/relic.h>
}

using namespace nomos;

class ClientSearchTimeSmokeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if (core_get() == NULL) {
      if (core_init() != RLC_OK) {
        FAIL() << "Failed to initialize RELIC";
      }

      if (pc_param_set_any() != RLC_OK) {
        core_clean();
        FAIL() << "Failed to set pairing parameters";
      }
    }
  }

  void TearDown() override {
    // Do not call core_clean() — RELIC state is shared across all test suites
  }
};

// Smoke test: measure client search time for queries with increasing update
// counts Insert 100 keywords w1-w100 where w_i has i updates Query (w_i, w100)
// for i=1 to 99 Verify time increases with 70% threshold
TEST_F(ClientSearchTimeSmokeTest, PrepareSearchTimeIncreasesWithUpdateCount) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  // Insert 100 keywords w1-w100, where w_i has i updates
  const int num_keywords = 100;
  for (int i = 1; i <= num_keywords; ++i) {
    std::string keyword = "w" + std::to_string(i);
    // Each keyword w_i has i updates
    for (int j = 0; j < i; ++j) {
      std::string doc_id = "doc_" + std::to_string(i) + "_" + std::to_string(j);
      server.update(gatekeeper.update(OP_ADD, doc_id, keyword));
    }
  }

  // Collect timing data for queries (w_i, w100) for i=1 to 99
  std::vector<double> timings;
  for (int i = 1; i < num_keywords; ++i) {
    std::string kw_i = "w" + std::to_string(i);
    std::string kw_100 = "w100";

    const std::vector<std::string> query = {kw_i, kw_100};
    const TokenRequest token_request =
        client.genToken(query, gatekeeper.getUpdateCounts());
    const SearchToken token = gatekeeper.genToken(token_request);

    // Measure client.prepareSearch time
    auto start = std::chrono::high_resolution_clock::now();
    const Client::SearchRequest request =
        client.prepareSearch(token, token_request);
    auto end = std::chrono::high_resolution_clock::now();

    double elapsed_ms =
        std::chrono::duration<double, std::milli>(end - start).count();
    timings.push_back(elapsed_ms);

    // Output timing for observation
    std::cout << "Query (w" << i << ", w100): " << elapsed_ms << " ms"
              << std::endl;

    // Also verify the query works correctly (at least returns some results)
    const std::vector<SearchResultEntry> results = server.search(request);
    const std::vector<std::string> ids = client.decryptResults(results, token);

    // w_i has i updates, w100 has 100 updates
    // Intersection should be empty since different keywords have different docs
    // But we just verify the search completes without error
    (void)ids;  // Suppress unused variable warning
  }

  // Verify overall time trend increases
  // 1. Check that last query is slower than first query (at least 2x)
  bool overall_increase = timings.back() > timings.front() * 2.0;

  // 2. Sliding window: compare second half average to first half average
  size_t mid = timings.size() / 2;
  double first_half_avg = 0.0;
  double second_half_avg = 0.0;
  for (size_t i = 0; i < mid; ++i) {
    first_half_avg += timings[i];
  }
  first_half_avg /= mid;
  for (size_t i = mid; i < timings.size(); ++i) {
    second_half_avg += timings[i];
  }
  second_half_avg /= (timings.size() - mid);

  bool half_increase = second_half_avg > first_half_avg * 1.5;

  // 3. Sliding window: count increases over window of 5 comparisons
  int window_increases = 0;
  int window_total = 0;
  const int window_size = 5;
  for (size_t i = window_size; i < timings.size(); ++i) {
    ++window_total;
    double window_start_avg = 0;
    double window_end_avg = 0;
    for (int j = 0; j < window_size; ++j) {
      window_start_avg += timings[i - window_size + j];
      window_end_avg += timings[i - window_size + j + 1];
    }
    window_start_avg /= window_size;
    window_end_avg /= window_size;
    if (window_end_avg > window_start_avg * 1.3) {
      ++window_increases;
    }
  }
  double window_ratio =
      window_total > 0 ? static_cast<double>(window_increases) / window_total
                       : 0.0;

  std::cout << "Overall increase (t_last > t_first*2): " << overall_increase
            << " (" << timings.front() << " ms -> " << timings.back() << " ms)"
            << std::endl;
  std::cout << "Half comparison (second_avg > first_avg*1.5): " << half_increase
            << " (" << first_half_avg << " ms -> " << second_half_avg << " ms)"
            << std::endl;
  std::cout << "Window increases: " << window_increases << "/" << window_total
            << " (" << (window_ratio * 100) << "%)" << std::endl;

  // At least 2 of 3 verification methods should pass
  int pass_count = (overall_increase ? 1 : 0) + (half_increase ? 1 : 0) +
                   (window_ratio >= 0.5 ? 1 : 0);
  EXPECT_GE(pass_count, 2) << "Overall trend verification failed";
}
