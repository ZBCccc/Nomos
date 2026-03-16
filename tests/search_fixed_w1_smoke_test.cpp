#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <string>

#include "nomos/Client.hpp"
#include "nomos/Gatekeeper.hpp"
#include "nomos/Server.hpp"

#include "mc-odxt/McOdxtClient.hpp"
#include "mc-odxt/McOdxtGatekeeper.hpp"
#include "mc-odxt/McOdxtServer.hpp"

#include "vq-nomos/Client.hpp"
#include "vq-nomos/Gatekeeper.hpp"
#include "vq-nomos/Server.hpp"

extern "C" {
#include <relic/relic.h>
}

using namespace nomos;

class SearchFixedW1SmokeTest : public ::testing::Test {
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
};

// Test Nomos timing and correctness
TEST_F(SearchFixedW1SmokeTest, NomosThreeWayTiming) {
  Gatekeeper gatekeeper;
  Client client;
  Server server;

  gatekeeper.setup(10);
  client.setup();
  server.setup(gatekeeper.getKm());

  std::string w1 = "keyword1";
  std::string w2 = "keyword2";
  
  // Insert 10 documents for w1, 100 for w2. Some overlap.
  for (int i = 0; i < 10; ++i) {
    server.update(gatekeeper.update(OP_ADD, "doc_shared_" + std::to_string(i), w1));
  }
  for (int i = 0; i < 100; ++i) {
    std::string doc_id = (i < 5) ? "doc_shared_" + std::to_string(i) : "doc_w2_" + std::to_string(i);
    server.update(gatekeeper.update(OP_ADD, doc_id, w2));
  }

  // Measure timings
  auto start_c1 = std::chrono::high_resolution_clock::now();
  TokenRequest token_req = client.genToken({w1, w2}, gatekeeper.getUpdateCounts());
  auto end_c1 = std::chrono::high_resolution_clock::now();

  auto start_g = std::chrono::high_resolution_clock::now();
  SearchToken token = gatekeeper.genToken(token_req);
  auto end_g = std::chrono::high_resolution_clock::now();

  auto start_c2 = std::chrono::high_resolution_clock::now();
  Client::SearchRequest req = client.prepareSearch(token, token_req);
  auto end_c2 = std::chrono::high_resolution_clock::now();

  auto start_s = std::chrono::high_resolution_clock::now();
  auto results = server.search(req);
  auto end_s = std::chrono::high_resolution_clock::now();

  auto start_c3 = std::chrono::high_resolution_clock::now();
  auto decrypted = client.decryptResults(results, token);
  auto end_c3 = std::chrono::high_resolution_clock::now();

  double client_total = std::chrono::duration<double, std::milli>((end_c1-start_c1) + (end_c2-start_c2) + (end_c3-start_c3)).count();
  double gatekeeper_total = std::chrono::duration<double, std::milli>(end_g-start_g).count();
  double server_total = std::chrono::duration<double, std::milli>(end_s-start_s).count();

  std::cout << "[Nomos Smoke] Client: " << client_total << "ms, GK: " << gatekeeper_total << "ms, Server: " << server_total << "ms" << std::endl;

  EXPECT_GT(client_total, 0);
  EXPECT_GT(gatekeeper_total, 0);
  EXPECT_GT(server_total, 0);
  // Shared docs are doc_shared_0 to 4 (intersection size 5)
  EXPECT_EQ(decrypted.size(), 5);
}

// Test MC-ODXT timing and correctness
TEST_F(SearchFixedW1SmokeTest, McOdxtThreeWayTiming) {
  mcodxt::McOdxtGatekeeper gatekeeper;
  mcodxt::McOdxtClient client;
  mcodxt::McOdxtServer server;

  gatekeeper.setup(10);
  client.setup();
  server.setup(gatekeeper.getKm());

  std::string w1 = "keyword1";
  std::string w2 = "keyword2";

  for (int i = 0; i < 10; ++i) {
    server.update(gatekeeper.update(mcodxt::OpType::ADD, "doc_shared_" + std::to_string(i), w1));
  }
  for (int i = 0; i < 100; ++i) {
    std::string doc_id = (i < 5) ? "doc_shared_" + std::to_string(i) : "doc_w2_" + std::to_string(i);
    server.update(gatekeeper.update(mcodxt::OpType::ADD, doc_id, w2));
  }

  auto start_c1 = std::chrono::high_resolution_clock::now();
  auto token_req = client.genToken({w1, w2}, gatekeeper.getUpdateCounts());
  auto end_c1 = std::chrono::high_resolution_clock::now();

  auto start_g = std::chrono::high_resolution_clock::now();
  auto token = gatekeeper.genToken(token_req);
  auto end_g = std::chrono::high_resolution_clock::now();

  auto start_c2 = std::chrono::high_resolution_clock::now();
  auto req = client.prepareSearch(token, token_req);
  auto end_c2 = std::chrono::high_resolution_clock::now();

  auto start_s = std::chrono::high_resolution_clock::now();
  auto results = server.search(req);
  auto end_s = std::chrono::high_resolution_clock::now();

  auto start_c3 = std::chrono::high_resolution_clock::now();
  auto decrypted = client.decryptResults(results, token);
  auto end_c3 = std::chrono::high_resolution_clock::now();

  double client_total = std::chrono::duration<double, std::milli>((end_c1-start_c1) + (end_c2-start_c2) + (end_c3-start_c3)).count();
  double gatekeeper_total = std::chrono::duration<double, std::milli>(end_g-start_g).count();
  double server_total = std::chrono::duration<double, std::milli>(end_s-start_s).count();

  std::cout << "[MC-ODXT Smoke] Client: " << client_total << "ms, GK: " << gatekeeper_total << "ms, Server: " << server_total << "ms" << std::endl;

  EXPECT_GT(client_total, 0);
  EXPECT_GT(gatekeeper_total, 0);
  EXPECT_GT(server_total, 0);
  EXPECT_EQ(decrypted.size(), 5);
}

// Test VQ-Nomos timing and correctness
TEST_F(SearchFixedW1SmokeTest, VQNomosThreeWayTiming) {
  vqnomos::Gatekeeper gatekeeper;
  vqnomos::Client client;
  vqnomos::Server server;

  const size_t qtree_cap = 1024;
  gatekeeper.setup(10, qtree_cap);
  auto anchor = gatekeeper.getCurrentAnchor();
  client.setup(gatekeeper.getPublicKeyPem(), anchor, qtree_cap, 10);
  server.setup(gatekeeper.getKm(), anchor, qtree_cap);

  std::string w1 = "keyword1";
  std::string w2 = "keyword2";

  for (int i = 0; i < 10; ++i) {
    server.update(gatekeeper.update(vqnomos::OP_ADD, "doc_shared_" + std::to_string(i), w1));
  }
  for (int i = 0; i < 100; ++i) {
    std::string doc_id = (i < 5) ? "doc_shared_" + std::to_string(i) : "doc_w2_" + std::to_string(i);
    server.update(gatekeeper.update(vqnomos::OP_ADD, doc_id, w2));
  }

  auto start_c1 = std::chrono::high_resolution_clock::now();
  auto token_req = client.genToken({w1, w2}, gatekeeper.getUpdateCounts());
  auto end_c1 = std::chrono::high_resolution_clock::now();

  auto start_g = std::chrono::high_resolution_clock::now();
  auto token = gatekeeper.genToken(token_req);
  auto end_g = std::chrono::high_resolution_clock::now();

  auto start_c2 = std::chrono::high_resolution_clock::now();
  auto req = client.prepareSearch(token, token_req);
  auto end_c2 = std::chrono::high_resolution_clock::now();

  auto start_s = std::chrono::high_resolution_clock::now();
  auto response = server.search(req, token);
  auto end_s = std::chrono::high_resolution_clock::now();

  auto start_c3 = std::chrono::high_resolution_clock::now();
  auto decrypted = client.decryptAndVerify(response, token, token_req);
  auto end_c3 = std::chrono::high_resolution_clock::now();

  double client_total = std::chrono::duration<double, std::milli>((end_c1-start_c1) + (end_c2-start_c2) + (end_c3-start_c3)).count();
  double gatekeeper_total = std::chrono::duration<double, std::milli>(end_g-start_g).count();
  double server_total = std::chrono::duration<double, std::milli>(end_s-start_s).count();

  std::cout << "[VQ-Nomos Smoke] Client: " << client_total << "ms, GK: " << gatekeeper_total << "ms, Server: " << server_total << "ms" << std::endl;

  EXPECT_GT(client_total, 0);
  EXPECT_GT(gatekeeper_total, 0);
  EXPECT_GT(server_total, 0);
  EXPECT_TRUE(decrypted.accepted);
  EXPECT_EQ(decrypted.ids.size(), 5);
}
