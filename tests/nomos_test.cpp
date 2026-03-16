#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "nomos/Client.hpp"
#include "nomos/Gatekeeper.hpp"
#include "nomos/Server.hpp"

extern "C" {
#include <relic/relic.h>
}

using namespace nomos;

class NomosTest : public ::testing::Test {
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

TEST_F(NomosTest, MultiKeywordSearchReturnsIntersection) {
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
  const TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  const SearchToken token = gatekeeper.genToken(token_request);
  const Client::SearchRequest request =
      client.prepareSearch(token, token_request);
  const std::vector<SearchResultEntry> results = server.search(request);
  const std::vector<std::string> ids = client.decryptResults(results, token);

  ASSERT_EQ(ids.size(), 1);
  EXPECT_EQ(ids[0], "doc1");
}

TEST_F(NomosTest, SingleKeywordSearchReturnsAllMatchingDocuments) {
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
  const TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  const SearchToken token = gatekeeper.genToken(token_request);
  const Client::SearchRequest request =
      client.prepareSearch(token, token_request);
  const std::vector<SearchResultEntry> results = server.search(request);
  std::vector<std::string> ids = client.decryptResults(results, token);

  std::sort(ids.begin(), ids.end());
  ASSERT_EQ(ids.size(), 2);
  EXPECT_EQ(ids[0], "doc1");
  EXPECT_EQ(ids[1], "doc3");
}

TEST_F(NomosTest, PrepareSearchUsesTokenSnapshotAfterInterleavedUpdate) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));

  const std::vector<std::string> query = {"crypto"};
  const TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  const SearchToken token = gatekeeper.genToken(token_request);
  ASSERT_EQ(token.bstag.size(), 1u);

  server.update(gatekeeper.update(OP_ADD, "doc2", "crypto"));

  const Client::SearchRequest request =
      client.prepareSearch(token, token_request);
  EXPECT_EQ(request.stokenList.size(), token.bstag.size());
  EXPECT_EQ(request.xtokenList.size(), token.bstag.size());

  const std::vector<SearchResultEntry> results = server.search(request);
  const std::vector<std::string> ids = client.decryptResults(results, token);

  ASSERT_EQ(ids.size(), 1u);
  EXPECT_EQ(ids[0], "doc1");
}

TEST_F(NomosTest, DeletedDocumentDoesNotAppearInResults) {
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
  const TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  const SearchToken token = gatekeeper.genToken(token_request);
  const Client::SearchRequest request =
      client.prepareSearch(token, token_request);
  const std::vector<SearchResultEntry> results = server.search(request);
  const std::vector<std::string> ids = client.decryptResults(results, token);

  ASSERT_EQ(ids.size(), 1u);
  EXPECT_EQ(ids[0], "doc2");
}

TEST_F(NomosTest, ReaddedDocumentAppearsAfterDelete) {
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
  const TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  const SearchToken token = gatekeeper.genToken(token_request);
  const Client::SearchRequest request =
      client.prepareSearch(token, token_request);
  const std::vector<SearchResultEntry> results = server.search(request);
  const std::vector<std::string> ids = client.decryptResults(results, token);

  ASSERT_EQ(ids.size(), 1u);
  EXPECT_EQ(ids[0], "doc1");
}

TEST_F(NomosTest, SearchForNeverUpdatedKeywordReturnsEmpty) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));

  const std::vector<std::string> query = {"nonexistent"};
  const TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  const SearchToken token = gatekeeper.genToken(token_request);
  const Client::SearchRequest request =
      client.prepareSearch(token, token_request);
  const std::vector<SearchResultEntry> results = server.search(request);
  const std::vector<std::string> ids = client.decryptResults(results, token);

  EXPECT_TRUE(ids.empty());
}

TEST_F(NomosTest, MultiKeywordQueryWithNonExistentKeywordReturnsEmpty) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OP_ADD, "doc1", "security"));

  const std::vector<std::string> query = {"crypto", "nonexistent"};
  const TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  const SearchToken token = gatekeeper.genToken(token_request);
  const Client::SearchRequest request =
      client.prepareSearch(token, token_request);
  const std::vector<SearchResultEntry> results = server.search(request);
  const std::vector<std::string> ids = client.decryptResults(results, token);

  EXPECT_TRUE(ids.empty());
}

TEST_F(NomosTest, GetUpdateCountReflectsAddAndDelete) {
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

TEST_F(NomosTest, UsesLeastFrequentKeywordAsPrimaryTerm) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OP_ADD, "doc2", "crypto"));
  server.update(gatekeeper.update(OP_ADD, "doc1", "security"));

  const std::vector<std::string> query = {"crypto", "security"};
  const TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  ASSERT_EQ(token_request.query_keywords.size(), 2u);
  EXPECT_EQ(token_request.query_keywords[0], "security");

  const SearchToken token = gatekeeper.genToken(token_request);
  EXPECT_EQ(token.bstag.size(), 1u);

  const Client::SearchRequest request =
      client.prepareSearch(token, token_request);
  EXPECT_EQ(request.stokenList.size(), 1u);

  const std::vector<SearchResultEntry> results = server.search(request);
  const std::vector<std::string> ids = client.decryptResults(results, token);

  ASSERT_EQ(ids.size(), 1u);
  EXPECT_EQ(ids[0], "doc1");
}

TEST_F(NomosTest, LargeScale1000Updates) {
  // Paper: Algorithm 3 – Search (Section 4.3).
  // 200 docs × 5 keyword tiers = 1000 insertions. Keyword update counts:
  //   "global"   : 200   "tier_Y" (Y=i%5)  : 40 each
  //   "bucket_X" (X=i/20): 20 each   "niche_Z" (Z=i%20): 10 each
  //   "cell_W"  (W=i%100): 2 each
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);
  Client client;
  ASSERT_EQ(client.setup(), 0);
  Server server;
  server.setup(gatekeeper.getKm());

  for (int doc_i = 0; doc_i < 200; ++doc_i) {
    std::ostringstream ss;
    ss << "doc" << doc_i;
    const std::string doc_id = ss.str();

    server.update(gatekeeper.update(OP_ADD, doc_id, "global"));
    {
      std::ostringstream kw;
      kw << "bucket_" << (doc_i / 20);
      server.update(gatekeeper.update(OP_ADD, doc_id, kw.str()));
    }
    {
      std::ostringstream kw;
      kw << "tier_" << (doc_i % 5);
      server.update(gatekeeper.update(OP_ADD, doc_id, kw.str()));
    }
    {
      std::ostringstream kw;
      kw << "niche_" << (doc_i % 20);
      server.update(gatekeeper.update(OP_ADD, doc_id, kw.str()));
    }
    {
      std::ostringstream kw;
      kw << "cell_" << (doc_i % 100);
      server.update(gatekeeper.update(OP_ADD, doc_id, kw.str()));
    }
  }
  // Total inserts: 200 × 5 = 1000

  // Single-keyword: "global" → all 200 docs
  {
    const std::vector<std::string> query = {"global"};
    const TokenRequest token_request =
        client.genToken(query, gatekeeper.getUpdateCounts());
    const SearchToken token = gatekeeper.genToken(token_request);
    const Client::SearchRequest request =
        client.prepareSearch(token, token_request);
    const std::vector<SearchResultEntry> results = server.search(request);
    const std::vector<std::string> ids = client.decryptResults(results, token);
    EXPECT_EQ(ids.size(), 200u);
  }

  // 2-keyword intersection: {"tier_0", "global"} → 40 docs (i%5==0)
  {
    const std::vector<std::string> query = {"tier_0", "global"};
    const TokenRequest token_request =
        client.genToken(query, gatekeeper.getUpdateCounts());
    const SearchToken token = gatekeeper.genToken(token_request);
    const Client::SearchRequest request =
        client.prepareSearch(token, token_request);
    const std::vector<SearchResultEntry> results = server.search(request);
    const std::vector<std::string> ids = client.decryptResults(results, token);
    EXPECT_EQ(ids.size(), 40u);
  }

  // 3-keyword intersection: {"niche_0","tier_0","global"} → 10 docs (i%20==0)
  {
    const std::vector<std::string> query = {"niche_0", "tier_0", "global"};
    const TokenRequest token_request =
        client.genToken(query, gatekeeper.getUpdateCounts());
    const SearchToken token = gatekeeper.genToken(token_request);
    const Client::SearchRequest request =
        client.prepareSearch(token, token_request);
    const std::vector<SearchResultEntry> results = server.search(request);
    const std::vector<std::string> ids = client.decryptResults(results, token);
    EXPECT_EQ(ids.size(), 10u);
  }
}
