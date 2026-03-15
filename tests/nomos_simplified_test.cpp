#include <gtest/gtest.h>

#include <algorithm>

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
    // in this process. Cleaning here breaks subsequent suites (McOdxtTest,
    // etc.)
  }
};

TEST_F(NomosSimplifiedTest, MultiKeywordSearchReturnsIntersection) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  const std::vector<std::pair<std::string, std::string>> updates = {
      {"doc1", "crypto"},  {"doc1", "security"}, {"doc2", "security"},
      {"doc2", "privacy"}, {"doc3", "crypto"},   {"doc3", "blockchain"}};

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

TEST_F(NomosSimplifiedTest,
       PrepareSearchUsesTokenSnapshotAfterInterleavedUpdate) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));

  const std::vector<std::string> query = {"crypto"};
  const SearchToken token = client.genTokenSimplified(query, gatekeeper);
  ASSERT_EQ(token.bstag.size(), 1u);

  server.update(gatekeeper.update(OP_ADD, "doc2", "crypto"));

  const Client::SearchRequest request =
      client.prepareSearch(token, query, gatekeeper.getUpdateCounts());
  EXPECT_EQ(request.stokenList.size(), token.bstag.size());
  EXPECT_EQ(request.xtokenList.size(), token.bstag.size());

  const std::vector<SearchResultEntry> results = server.search(request);
  const std::vector<std::string> ids = client.decryptResults(results, token);

  ASSERT_EQ(ids.size(), 1u);
  EXPECT_EQ(ids[0], "doc1");
}

TEST_F(NomosSimplifiedTest, DeletedDocumentDoesNotAppearInResults) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OP_ADD, "doc2", "crypto"));
  server.update(gatekeeper.update(OP_DEL, "doc1", "crypto"));

  const std::vector<std::string> query = {"crypto"};
  const SearchToken token = client.genTokenSimplified(query, gatekeeper);
  const Client::SearchRequest request =
      client.prepareSearch(token, query, gatekeeper.getUpdateCounts());
  const std::vector<SearchResultEntry> results = server.search(request);
  const std::vector<std::string> ids = client.decryptResults(results, token);

  ASSERT_EQ(ids.size(), 1u);
  EXPECT_EQ(ids[0], "doc2");
}

TEST_F(NomosSimplifiedTest, ReaddedDocumentAppearsAfterDelete) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OP_DEL, "doc1", "crypto"));
  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));

  const std::vector<std::string> query = {"crypto"};
  const SearchToken token = client.genTokenSimplified(query, gatekeeper);
  const Client::SearchRequest request =
      client.prepareSearch(token, query, gatekeeper.getUpdateCounts());
  const std::vector<SearchResultEntry> results = server.search(request);
  const std::vector<std::string> ids = client.decryptResults(results, token);

  ASSERT_EQ(ids.size(), 1u);
  EXPECT_EQ(ids[0], "doc1");
}

TEST_F(NomosSimplifiedTest, SearchForNeverUpdatedKeywordReturnsEmpty) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));

  const std::vector<std::string> query = {"nonexistent"};
  const SearchToken token = client.genTokenSimplified(query, gatekeeper);
  const Client::SearchRequest request =
      client.prepareSearch(token, query, gatekeeper.getUpdateCounts());
  const std::vector<SearchResultEntry> results = server.search(request);
  const std::vector<std::string> ids = client.decryptResults(results, token);

  EXPECT_TRUE(ids.empty());
}

TEST_F(NomosSimplifiedTest,
       MultiKeywordQueryWithNonExistentKeywordReturnsEmpty) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OP_ADD, "doc1", "security"));

  const std::vector<std::string> query = {"crypto", "nonexistent"};
  const SearchToken token = client.genTokenSimplified(query, gatekeeper);
  const Client::SearchRequest request =
      client.prepareSearch(token, query, gatekeeper.getUpdateCounts());
  const std::vector<SearchResultEntry> results = server.search(request);
  const std::vector<std::string> ids = client.decryptResults(results, token);

  EXPECT_TRUE(ids.empty());
}

TEST_F(NomosSimplifiedTest, GetUpdateCountReflectsAddAndDelete) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  EXPECT_EQ(gatekeeper.getUpdateCount("crypto"), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));
  EXPECT_EQ(gatekeeper.getUpdateCount("crypto"), 1);

  server.update(gatekeeper.update(OP_ADD, "doc2", "crypto"));
  EXPECT_EQ(gatekeeper.getUpdateCount("crypto"), 2);

  server.update(gatekeeper.update(OP_DEL, "doc1", "crypto"));
  EXPECT_EQ(gatekeeper.getUpdateCount("crypto"), 3);
}
