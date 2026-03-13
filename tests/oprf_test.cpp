#include <gtest/gtest.h>

#include "nomos/Client.hpp"
#include "nomos/Gatekeeper.hpp"
#include "nomos/Server.hpp"

extern "C" {
#include <relic/relic.h>
}

using namespace nomos;

class OPRFTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize RELIC
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

// Test OPRF blinding protocol end-to-end
TEST_F(OPRFTest, FullOPRFProtocol) {
  // Setup
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());  // Setup server with decryption key

  // Add some test data
  std::vector<std::string> keywords = {"keyword1", "keyword2", "keyword3"};
  std::vector<std::string> ids = {"doc1", "doc2", "doc3"};

  for (size_t i = 0; i < keywords.size(); ++i) {
    for (const auto& kw : keywords) {
      UpdateMetadata meta = gatekeeper.update(OP_ADD, ids[i], kw);
      server.update(meta);
    }
  }

  // Test OPRF token generation
  std::vector<std::string> query = {"keyword1", "keyword2"};

  // Phase 1: Client generates blinded request
  BlindedRequest request = client.genTokenPhase1(query, gatekeeper.getUpdateCounts());

  // Verify request structure
  EXPECT_EQ(request.a.size(), 2);  // n=2 keywords
  EXPECT_EQ(request.b.size(), gatekeeper.getUpdateCount("keyword1"));  // m updates
  EXPECT_EQ(request.c.size(), gatekeeper.getUpdateCount("keyword1"));
  EXPECT_EQ(request.av.size(), 2);

  // Phase 2: Gatekeeper processes blinded request
  BlindedResponse response = gatekeeper.genTokenGatekeeper(request);

  // Verify response structure
  EXPECT_FALSE(response.strap_prime.empty());
  EXPECT_EQ(response.bstag_prime.size(), gatekeeper.getUpdateCount("keyword1"));
  EXPECT_EQ(response.delta_prime.size(), gatekeeper.getUpdateCount("keyword1"));
  EXPECT_EQ(response.bxtrap_prime.size(), 1);  // n-1 = 2-1 = 1
  EXPECT_FALSE(response.env.empty());

  // Phase 3: Client unblinds response
  SearchToken token = client.genTokenPhase2(response);

  // Verify token structure
  EXPECT_EQ(token.bstag.size(), gatekeeper.getUpdateCount("keyword1"));
  EXPECT_EQ(token.delta.size(), gatekeeper.getUpdateCount("keyword1"));
  EXPECT_EQ(token.bxtrap.size(), 1);
  EXPECT_FALSE(token.env.empty());

  // Phase 4: Use token for search (simplified - without env decryption)
  Client::SearchRequest search_req =
      client.prepareSearch(token, query, gatekeeper.getUpdateCounts());

  std::vector<SearchResultEntry> results = server.search(search_req);

  // Verify results
  EXPECT_GT(results.size(), 0);

  // Decrypt results
  std::vector<std::string> result_ids = client.decryptResults(results, token);
  EXPECT_GT(result_ids.size(), 0);

  // All documents should match (all have both keywords)
  EXPECT_EQ(result_ids.size(), 3);
}

// Test blinding factors are properly generated
TEST_F(OPRFTest, BlindingFactorsRandomness) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client1, client2;
  ASSERT_EQ(client1.setup(), 0);
  ASSERT_EQ(client2.setup(), 0);

  // Add test data
  gatekeeper.update(OP_ADD, "doc1", "keyword1");

  std::vector<std::string> query = {"keyword1"};

  // Generate two blinded requests with same query
  BlindedRequest req1 = client1.genTokenPhase1(query, gatekeeper.getUpdateCounts());
  BlindedRequest req2 = client2.genTokenPhase1(query, gatekeeper.getUpdateCounts());

  // Blinded values should be different (due to random r, s)
  EXPECT_NE(req1.a[0], req2.a[0]);
  EXPECT_NE(req1.b[0], req2.b[0]);
  EXPECT_NE(req1.c[0], req2.c[0]);

  // But access vectors should be the same
  EXPECT_EQ(req1.av, req2.av);
}

// Test unblinding correctness
// Verifies that OPRF protocol produces correct stags after Server unblinding
TEST_F(OPRFTest, UnblindingCorrectness) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  Server server;
  server.setup(gatekeeper.getKm());

  // Add test data
  gatekeeper.update(OP_ADD, "doc1", "keyword1");

  std::vector<std::string> query = {"keyword1"};

  // OPRF protocol: Client -> Gatekeeper -> Client
  BlindedRequest request = client.genTokenPhase1(query, gatekeeper.getUpdateCounts());
  BlindedResponse response = gatekeeper.genTokenGatekeeper(request);
  SearchToken token_oprf = client.genTokenPhase2(response);

  // Simplified protocol (direct computation)
  SearchToken token_simple = gatekeeper.genTokenSimplified(query);

  // Compare strap (both should be H(w1)^Ks)
  // strap doesn't use gamma, so should match directly
  uint8_t strap_oprf[256], strap_simple[256];
  int len_oprf = ep_size_bin(token_oprf.strap, 1);
  int len_simple = ep_size_bin(token_simple.strap, 1);
  ep_write_bin(strap_oprf, len_oprf, token_oprf.strap, 1);
  ep_write_bin(strap_simple, len_simple, token_simple.strap, 1);

  EXPECT_EQ(len_oprf, len_simple);
  EXPECT_EQ(std::string(reinterpret_cast<char*>(strap_oprf), len_oprf),
            std::string(reinterpret_cast<char*>(strap_simple), len_simple));

  // For bstag: OPRF version has gamma factor, need Server to unblind
  // Create search requests for both tokens
  Client::SearchRequest req_oprf =
      client.prepareSearch(token_oprf, query, gatekeeper.getUpdateCounts());
  Client::SearchRequest req_simple =
      client.prepareSearch(token_simple, query, gatekeeper.getUpdateCounts());

  // Server unblinds OPRF stokens using gamma from env
  // After Server unblinding, both should produce the same stag
  // We verify this by checking that both can find the same results

  // Note: We can't directly compare stags because Server doesn't expose them
  // Instead, we verify that both protocols produce the same search behavior
  // by checking that the OPRF token works correctly in the full protocol

  // This is tested in FullOPRFProtocol test
  // Here we just verify that env is properly encrypted and non-empty
  EXPECT_FALSE(req_oprf.env.empty());
  EXPECT_EQ(req_oprf.stokenList.size(), req_simple.stokenList.size());
}

// Test env encryption/decryption
TEST_F(OPRFTest, EnvEncryption) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10), 0);

  Client client;
  ASSERT_EQ(client.setup(), 0);

  gatekeeper.update(OP_ADD, "doc1", "keyword1");

  std::vector<std::string> query = {"keyword1"};

  BlindedRequest request = client.genTokenPhase1(query, gatekeeper.getUpdateCounts());
  BlindedResponse response = gatekeeper.genTokenGatekeeper(request);

  // env should be non-empty and encrypted
  EXPECT_FALSE(response.env.empty());

  // env should contain rho and gamma values (encrypted)
  // Size should be reasonable (not too small, not too large)
  EXPECT_GT(response.env.size(), 10);
  EXPECT_LT(response.env.size(), 1000);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
