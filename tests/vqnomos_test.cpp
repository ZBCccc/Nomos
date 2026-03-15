#include <gtest/gtest.h>

#include <vector>

#include "vq-nomos/Client.hpp"
#include "vq-nomos/Gatekeeper.hpp"
#include "vq-nomos/Server.hpp"

extern "C" {
#include <relic/relic.h>
}

using namespace vqnomos;

class VQNomosTest : public ::testing::Test {
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

TEST_F(VQNomosTest, MultiKeywordSearchVerifiesAndReturnsIntersection) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10, 1024), 0);

  const Anchor initial_anchor = gatekeeper.getCurrentAnchor();
  Client client;
  ASSERT_EQ(
      client.setup(gatekeeper.getPublicKeyPem(), initial_anchor, 1024, 10), 0);

  Server server;
  server.setup(gatekeeper.getKm(), initial_anchor, 1024);

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OP_ADD, "doc1", "security"));
  server.update(gatekeeper.update(OP_ADD, "doc2", "security"));
  server.update(gatekeeper.update(OP_ADD, "doc3", "crypto"));

  const std::vector<std::string> query = {"crypto", "security"};
  const TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  const SearchToken token = gatekeeper.genToken(token_request);
  const SearchRequest request = client.prepareSearch(token, token_request);
  const SearchResponse response = server.search(request, token);
  const VerificationResult result =
      client.decryptAndVerify(response, token, token_request);

  ASSERT_TRUE(result.accepted);
  ASSERT_EQ(result.ids.size(), 1u);
  EXPECT_EQ(result.ids[0], "doc1");
}

TEST_F(VQNomosTest, TamperedAnchorIsRejected) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10, 1024), 0);

  const Anchor initial_anchor = gatekeeper.getCurrentAnchor();
  Client client;
  ASSERT_EQ(
      client.setup(gatekeeper.getPublicKeyPem(), initial_anchor, 1024, 10), 0);

  Server server;
  server.setup(gatekeeper.getKm(), initial_anchor, 1024);

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OP_ADD, "doc1", "security"));

  const std::vector<std::string> query = {"crypto", "security"};
  const TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  const SearchToken token = gatekeeper.genToken(token_request);
  const SearchRequest request = client.prepareSearch(token, token_request);
  SearchResponse response = server.search(request, token);
  response.anchor.root_hash = "bad-anchor";

  const VerificationResult result =
      client.decryptAndVerify(response, token, token_request);
  EXPECT_FALSE(result.accepted);
}

TEST_F(VQNomosTest, TamperedQTreeWitnessIsRejected) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10, 1024), 0);

  const Anchor initial_anchor = gatekeeper.getCurrentAnchor();
  Client client;
  ASSERT_EQ(
      client.setup(gatekeeper.getPublicKeyPem(), initial_anchor, 1024, 10), 0);

  Server server;
  server.setup(gatekeeper.getKm(), initial_anchor, 1024);

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OP_ADD, "doc1", "security"));

  const std::vector<std::string> query = {"crypto", "security"};
  const TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  const SearchToken token = gatekeeper.genToken(token_request);
  const SearchRequest request = client.prepareSearch(token, token_request);
  SearchResponse response = server.search(request, token);
  ASSERT_FALSE(response.relation_proofs.empty());
  ASSERT_FALSE(response.relation_proofs[0].qualification.witnesses.empty());
  response.relation_proofs[0].qualification.witnesses[0].address = "bad-proof";

  const VerificationResult result =
      client.decryptAndVerify(response, token, token_request);
  EXPECT_FALSE(result.accepted);
}
