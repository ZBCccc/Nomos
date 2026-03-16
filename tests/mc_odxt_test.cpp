#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "mc-odxt/McOdxtClient.hpp"
#include "mc-odxt/McOdxtGatekeeper.hpp"
#include "mc-odxt/McOdxtServer.hpp"
#include "mc-odxt/McOdxtTypes.hpp"
#include "nomos/Client.hpp"
#include "nomos/Gatekeeper.hpp"
#include "nomos/Server.hpp"

extern "C" {
#include <relic/relic.h>
}

using namespace mcodxt;

class McOdxtTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Only initialize RELIC if not already initialized
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
    // Don't clean up RELIC - it's shared across tests
  }
};

static std::vector<std::string> sorted(std::vector<std::string> ids) {
  std::sort(ids.begin(), ids.end());
  return ids;
}

TEST_F(McOdxtTest, SingleKeywordSearch) {
  McOdxtGatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);
  McOdxtServer server;
  server.setup(gatekeeper.getKm());
  McOdxtClient client;
  ASSERT_EQ(client.setup(), 0);

  server.update(gatekeeper.update(OpType::ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OpType::ADD, "doc2", "crypto"));

  const std::vector<std::string> query = {"crypto"};
  TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  SearchToken token = gatekeeper.genToken(token_request);
  McOdxtClient::SearchRequest req = client.prepareSearch(token, token_request);
  std::vector<SearchResultEntry> results = server.search(req);
  std::vector<std::string> ids = sorted(client.decryptResults(results, token));

  ASSERT_EQ(gatekeeper.getUpdateCount("crypto"), 2);
  ASSERT_EQ(ids.size(), 2u);
  EXPECT_EQ(ids[0], "doc1");
  EXPECT_EQ(ids[1], "doc2");
}

TEST_F(McOdxtTest, MultiKeywordIntersection) {
  McOdxtGatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);
  McOdxtServer server;
  server.setup(gatekeeper.getKm());
  McOdxtClient client;
  ASSERT_EQ(client.setup(), 0);

  server.update(gatekeeper.update(OpType::ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OpType::ADD, "doc1", "security"));
  server.update(gatekeeper.update(OpType::ADD, "doc2", "crypto"));
  server.update(gatekeeper.update(OpType::ADD, "doc3", "security"));

  const std::vector<std::string> query = {"crypto", "security"};
  TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  SearchToken token = gatekeeper.genToken(token_request);
  McOdxtClient::SearchRequest req = client.prepareSearch(token, token_request);
  std::vector<SearchResultEntry> results = server.search(req);
  std::vector<std::string> ids = sorted(client.decryptResults(results, token));

  ASSERT_EQ(ids.size(), 1u);
  EXPECT_EQ(ids[0], "doc1");
}

TEST_F(McOdxtTest, UsesLeastFrequentKeywordAsPrimaryTerm) {
  McOdxtGatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);
  McOdxtServer server;
  server.setup(gatekeeper.getKm());
  McOdxtClient client;
  ASSERT_EQ(client.setup(), 0);

  server.update(gatekeeper.update(OpType::ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OpType::ADD, "doc2", "crypto"));
  server.update(gatekeeper.update(OpType::ADD, "doc1", "security"));

  const std::vector<std::string> query = {"crypto", "security"};
  TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  ASSERT_EQ(token_request.query_keywords.size(), 2u);
  EXPECT_EQ(token_request.query_keywords[0], "security");

  SearchToken token = gatekeeper.genToken(token_request);
  EXPECT_EQ(token.bstag.size(), 1u);

  McOdxtClient::SearchRequest req = client.prepareSearch(token, token_request);
  EXPECT_EQ(req.stokenList.size(), 1u);

  std::vector<SearchResultEntry> results = server.search(req);
  std::vector<std::string> ids = sorted(client.decryptResults(results, token));

  ASSERT_EQ(ids.size(), 1u);
  EXPECT_EQ(ids[0], "doc1");
}

TEST_F(McOdxtTest, BothReturnSameIntersectionForSimpleDataset) {
  // MC-ODXT and Nomos use independent keys and different xtag counts per entry
  // (MC-ODXT: 1 xtag per (keyword, doc) pair; Nomos: ℓ=3 xtags).
  // This test only verifies that both protocols produce identical result sets
  // on the same dataset — it does not test protocol equivalence.
  McOdxtGatekeeper mc_gatekeeper;
  ASSERT_EQ(mc_gatekeeper.setup(10), 0);
  McOdxtServer mc_server;
  mc_server.setup(mc_gatekeeper.getKm());
  McOdxtClient mc_client;
  ASSERT_EQ(mc_client.setup(), 0);

  nomos::Gatekeeper nomos_gatekeeper;
  ASSERT_EQ(nomos_gatekeeper.setup(10), 0);
  nomos::Server nomos_server;
  nomos_server.setup(nomos_gatekeeper.getKm());
  nomos::Client nomos_client;
  ASSERT_EQ(nomos_client.setup(), 0);

  struct UpdateCase {
    const char* doc_id;
    const char* keyword;
  };
  const UpdateCase updates[] = {{"doc1", "crypto"},   {"doc1", "security"},
                                {"doc2", "security"}, {"doc2", "privacy"},
                                {"doc3", "crypto"},   {"doc3", "blockchain"}};

  for (size_t i = 0; i < sizeof(updates) / sizeof(updates[0]); ++i) {
    mc_server.update(mc_gatekeeper.update(OpType::ADD, updates[i].doc_id,
                                          updates[i].keyword));
    nomos_server.update(nomos_gatekeeper.update(
        nomos::OP_ADD, updates[i].doc_id, updates[i].keyword));
  }

  const std::vector<std::string> query = {"crypto", "security"};

  TokenRequest mc_token_request =
      mc_client.genToken(query, mc_gatekeeper.getUpdateCounts());
  SearchToken mc_token = mc_gatekeeper.genToken(mc_token_request);
  McOdxtClient::SearchRequest mc_req =
      mc_client.prepareSearch(mc_token, mc_token_request);
  std::vector<SearchResultEntry> mc_results = mc_server.search(mc_req);
  std::vector<std::string> mc_ids =
      sorted(mc_client.decryptResults(mc_results, mc_token));

  nomos::TokenRequest nomos_token_request =
      nomos_client.genToken(query, nomos_gatekeeper.getUpdateCounts());
  nomos::SearchToken nomos_token =
      nomos_gatekeeper.genToken(nomos_token_request);
  nomos::Client::SearchRequest nomos_req =
      nomos_client.prepareSearch(nomos_token, nomos_token_request);
  std::vector<nomos::SearchResultEntry> nomos_results =
      nomos_server.search(nomos_req);
  std::vector<std::string> nomos_ids =
      sorted(nomos_client.decryptResults(nomos_results, nomos_token));

  EXPECT_EQ(mc_ids, nomos_ids);
}

TEST_F(McOdxtTest, PrepareSearchUsesTokenSnapshotAfterInterleavedUpdate) {
  McOdxtGatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);
  McOdxtServer server;
  server.setup(gatekeeper.getKm());
  McOdxtClient client;
  ASSERT_EQ(client.setup(), 0);

  server.update(gatekeeper.update(OpType::ADD, "doc1", "crypto"));

  const std::vector<std::string> query = {"crypto"};
  TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  SearchToken token = gatekeeper.genToken(token_request);
  ASSERT_EQ(token.bstag.size(), 1u);

  server.update(gatekeeper.update(OpType::ADD, "doc2", "crypto"));

  McOdxtClient::SearchRequest req = client.prepareSearch(token, token_request);
  EXPECT_EQ(req.stokenList.size(), token.bstag.size());
  EXPECT_EQ(req.xtokenList.size(), token.bstag.size());

  std::vector<SearchResultEntry> results = server.search(req);
  std::vector<std::string> ids = sorted(client.decryptResults(results, token));

  ASSERT_EQ(ids.size(), 1u);
  EXPECT_EQ(ids[0], "doc1");
}

TEST_F(McOdxtTest, DeletedDocumentDoesNotAppearInResults) {
  McOdxtGatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);
  McOdxtServer server;
  server.setup(gatekeeper.getKm());
  McOdxtClient client;
  ASSERT_EQ(client.setup(), 0);

  server.update(gatekeeper.update(OpType::ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OpType::ADD, "doc2", "crypto"));
  server.update(gatekeeper.update(OpType::DEL, "doc1", "crypto"));

  const std::vector<std::string> query = {"crypto"};
  TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  SearchToken token = gatekeeper.genToken(token_request);
  McOdxtClient::SearchRequest req = client.prepareSearch(token, token_request);
  std::vector<SearchResultEntry> results = server.search(req);
  std::vector<std::string> ids = sorted(client.decryptResults(results, token));

  ASSERT_EQ(ids.size(), 1u);
  EXPECT_EQ(ids[0], "doc2");
}

TEST_F(McOdxtTest, ReaddedDocumentAppearsAfterDelete) {
  McOdxtGatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);
  McOdxtServer server;
  server.setup(gatekeeper.getKm());
  McOdxtClient client;
  ASSERT_EQ(client.setup(), 0);

  server.update(gatekeeper.update(OpType::ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OpType::DEL, "doc1", "crypto"));
  server.update(gatekeeper.update(OpType::ADD, "doc1", "crypto"));

  const std::vector<std::string> query = {"crypto"};
  TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  SearchToken token = gatekeeper.genToken(token_request);
  McOdxtClient::SearchRequest req = client.prepareSearch(token, token_request);
  std::vector<SearchResultEntry> results = server.search(req);
  std::vector<std::string> ids = sorted(client.decryptResults(results, token));

  ASSERT_EQ(ids.size(), 1u);
  EXPECT_EQ(ids[0], "doc1");
}

TEST_F(McOdxtTest, SearchForNeverUpdatedKeywordReturnsEmpty) {
  McOdxtGatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);
  McOdxtServer server;
  server.setup(gatekeeper.getKm());
  McOdxtClient client;
  ASSERT_EQ(client.setup(), 0);

  server.update(gatekeeper.update(OpType::ADD, "doc1", "crypto"));

  const std::vector<std::string> query = {"nonexistent"};
  TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  SearchToken token = gatekeeper.genToken(token_request);
  McOdxtClient::SearchRequest req = client.prepareSearch(token, token_request);
  std::vector<SearchResultEntry> results = server.search(req);
  std::vector<std::string> ids = sorted(client.decryptResults(results, token));

  EXPECT_TRUE(ids.empty());
}

TEST_F(McOdxtTest, GetUpdateCountReflectsAddAndDelete) {
  McOdxtGatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);
  McOdxtServer server;
  server.setup(gatekeeper.getKm());

  EXPECT_EQ(gatekeeper.getUpdateCount("crypto"), 0);

  server.update(gatekeeper.update(OpType::ADD, "doc1", "crypto"));
  EXPECT_EQ(gatekeeper.getUpdateCount("crypto"), 1);

  server.update(gatekeeper.update(OpType::ADD, "doc2", "crypto"));
  EXPECT_EQ(gatekeeper.getUpdateCount("crypto"), 2);

  server.update(gatekeeper.update(OpType::DEL, "doc1", "crypto"));
  EXPECT_EQ(gatekeeper.getUpdateCount("crypto"), 3);
}

TEST_F(McOdxtTest, LargeScale1000Updates) {
  // Paper: MC-ODXT protocol (Section 5).
  // 200 docs × 5 keyword tiers = 1000 insertions. Same keyword structure as
  // NomosSimplifiedTest::LargeScale1000Updates for cross-scheme comparability.
  McOdxtGatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);
  McOdxtServer server;
  server.setup(gatekeeper.getKm());
  McOdxtClient client;
  ASSERT_EQ(client.setup(), 0);

  for (int doc_i = 0; doc_i < 200; ++doc_i) {
    std::ostringstream ss;
    ss << "doc" << doc_i;
    const std::string doc_id = ss.str();

    server.update(gatekeeper.update(OpType::ADD, doc_id, "global"));
    {
      std::ostringstream kw;
      kw << "bucket_" << (doc_i / 20);
      server.update(gatekeeper.update(OpType::ADD, doc_id, kw.str()));
    }
    {
      std::ostringstream kw;
      kw << "tier_" << (doc_i % 5);
      server.update(gatekeeper.update(OpType::ADD, doc_id, kw.str()));
    }
    {
      std::ostringstream kw;
      kw << "niche_" << (doc_i % 20);
      server.update(gatekeeper.update(OpType::ADD, doc_id, kw.str()));
    }
    {
      std::ostringstream kw;
      kw << "cell_" << (doc_i % 100);
      server.update(gatekeeper.update(OpType::ADD, doc_id, kw.str()));
    }
  }
  // Total inserts: 200 × 5 = 1000

  // Single-keyword: "global" → all 200 docs
  {
    const std::vector<std::string> query = {"global"};
    const TokenRequest token_request =
        client.genToken(query, gatekeeper.getUpdateCounts());
    const SearchToken token = gatekeeper.genToken(token_request);
    const McOdxtClient::SearchRequest request =
        client.prepareSearch(token, token_request);
    const std::vector<SearchResultEntry> results = server.search(request);
    const std::vector<std::string> ids =
        sorted(client.decryptResults(results, token));
    EXPECT_EQ(ids.size(), 200u);
  }

  // 2-keyword intersection: {"tier_0", "global"} → 40 docs (i%5==0)
  {
    const std::vector<std::string> query = {"tier_0", "global"};
    const TokenRequest token_request =
        client.genToken(query, gatekeeper.getUpdateCounts());
    const SearchToken token = gatekeeper.genToken(token_request);
    const McOdxtClient::SearchRequest request =
        client.prepareSearch(token, token_request);
    const std::vector<SearchResultEntry> results = server.search(request);
    const std::vector<std::string> ids =
        sorted(client.decryptResults(results, token));
    EXPECT_EQ(ids.size(), 40u);
  }

  // 3-keyword intersection: {"niche_0","tier_0","global"} → 10 docs (i%20==0)
  {
    const std::vector<std::string> query = {"niche_0", "tier_0", "global"};
    const TokenRequest token_request =
        client.genToken(query, gatekeeper.getUpdateCounts());
    const SearchToken token = gatekeeper.genToken(token_request);
    const McOdxtClient::SearchRequest request =
        client.prepareSearch(token, token_request);
    const std::vector<SearchResultEntry> results = server.search(request);
    const std::vector<std::string> ids =
        sorted(client.decryptResults(results, token));
    EXPECT_EQ(ids.size(), 10u);
  }
}
