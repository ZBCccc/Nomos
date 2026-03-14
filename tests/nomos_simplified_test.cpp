#include <algorithm>

#include <gtest/gtest.h>

#include "nomos/Client.hpp"
#include "nomos/Gatekeeper.hpp"
#include "nomos/Server.hpp"

extern "C" {
#include <relic/relic.h>
}

using namespace nomos;

class NomosSimplifiedTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if (core_init() != RLC_OK) {
      core_clean();
      FAIL() << "Failed to initialize RELIC";
    }

    if (pc_param_set_any() != RLC_OK) {
      FAIL() << "Failed to set pairing parameters";
    }
  }

  void TearDown() override { core_clean(); }
};

TEST_F(NomosSimplifiedTest, MultiKeywordSearchReturnsIntersection) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  const std::vector<std::pair<std::string, std::string>> updates = {
      {"doc1", "crypto"}, {"doc1", "security"}, {"doc2", "security"},
      {"doc2", "privacy"}, {"doc3", "crypto"},  {"doc3", "blockchain"}};

  for (const auto& update : updates) {
    const UpdateMetadata meta =
        gatekeeper.update(OP_ADD, update.first, update.second);
    server.update(meta);
  }

  const std::vector<std::string> query = {"crypto", "security"};
  const SearchToken token = client.genTokenSimplified(query, gatekeeper);
  const Client::SearchRequest request =
      client.prepareSearch(token, query, gatekeeper.getUpdateCounts());
  const std::vector<SearchResultEntry> results = server.search(request);
  const std::vector<std::string> ids = client.decryptResults(results, token);

  ASSERT_EQ(ids.size(), 1);
  EXPECT_EQ(ids[0], "doc1");
}

TEST_F(NomosSimplifiedTest, SingleKeywordSearchReturnsAllMatchingDocuments) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  const std::vector<std::pair<std::string, std::string>> updates = {
      {"doc1", "crypto"}, {"doc2", "privacy"}, {"doc3", "crypto"}};

  for (const auto& update : updates) {
    const UpdateMetadata meta =
        gatekeeper.update(OP_ADD, update.first, update.second);
    server.update(meta);
  }

  const std::vector<std::string> query = {"crypto"};
  const SearchToken token = client.genTokenSimplified(query, gatekeeper);
  const Client::SearchRequest request =
      client.prepareSearch(token, query, gatekeeper.getUpdateCounts());
  const std::vector<SearchResultEntry> results = server.search(request);
  std::vector<std::string> ids = client.decryptResults(results, token);

  std::sort(ids.begin(), ids.end());
  ASSERT_EQ(ids.size(), 2);
  EXPECT_EQ(ids[0], "doc1");
  EXPECT_EQ(ids[1], "doc3");
}
