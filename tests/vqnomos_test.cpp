#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "vq-nomos/Client.hpp"
#include "vq-nomos/Gatekeeper.hpp"
#include "vq-nomos/QTree.hpp"
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

TEST_F(VQNomosTest, CollisionOnlyQTreeHitDoesNotProduceFalseRejection) {
  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10, 1), 0);

  const Anchor initial_anchor = gatekeeper.getCurrentAnchor();
  Client client;
  ASSERT_EQ(client.setup(gatekeeper.getPublicKeyPem(), initial_anchor, 1, 10),
            0);

  Server server;
  server.setup(gatekeeper.getKm(), initial_anchor, 1);

  server.update(gatekeeper.update(OP_ADD, "doc1", "crypto"));
  server.update(gatekeeper.update(OP_ADD, "doc1", "security"));
  server.update(gatekeeper.update(OP_ADD, "doc2", "crypto"));

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

TEST_F(VQNomosTest, MissingMerkleAuthForPositiveProofIsRejected) {
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
  server.update(gatekeeper.update(OP_ADD, "doc2", "crypto"));

  const std::vector<std::string> query = {"crypto", "security"};
  const TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  const SearchToken token = gatekeeper.genToken(token_request);
  const SearchRequest request = client.prepareSearch(token, token_request);
  SearchResponse response = server.search(request, token);

  ASSERT_FALSE(response.relation_proofs.empty());
  bool found_positive = false;
  int censored_slot = 0;
  for (size_t i = 0; i < response.relation_proofs.size(); ++i) {
    RelationProof& proof = response.relation_proofs[i];
    if (!proof.qualification.verdict) {
      continue;
    }
    proof.has_auth = false;
    proof.openings.clear();
    censored_slot = proof.candidate_slot;
    found_positive = true;
    break;
  }

  ASSERT_TRUE(found_positive);
  response.result_slots.erase(
      std::remove(response.result_slots.begin(), response.result_slots.end(),
                  censored_slot),
      response.result_slots.end());

  const VerificationResult result =
      client.decryptAndVerify(response, token, token_request);
  EXPECT_FALSE(result.accepted);
}

TEST_F(VQNomosTest, NegativeProofWithMerkleAuthUsesSubsetCheck) {
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
  server.update(gatekeeper.update(OP_ADD, "doc2", "crypto"));

  const std::vector<std::string> query = {"crypto", "security"};
  const TokenRequest token_request =
      client.genToken(query, gatekeeper.getUpdateCounts());
  SearchToken token = gatekeeper.genToken(token_request);
  const SearchRequest request = client.prepareSearch(token, token_request);
  SearchResponse response = server.search(request, token);

  ASSERT_EQ(response.entries.size(), 1u);
  ASSERT_EQ(response.relation_proofs.size(), 1u);
  RelationProof& proof = response.relation_proofs[0];
  ASSERT_TRUE(proof.qualification.verdict);
  ASSERT_TRUE(proof.has_auth);
  ASSERT_FALSE(proof.openings.empty());

  // Verifier-level regression: use the signed all-zero anchor to realize the
  // paper's Negative+Auth branch with a valid subset proof.
  QTree empty_qtree(1024);
  empty_qtree.initialize(std::vector<bool>(1024, false));
  const std::string opened_xtag = proof.openings[0].xtag;

  QTreeWitness negative_witness;
  negative_witness.address = opened_xtag;
  negative_witness.bit_value = false;
  negative_witness.path = empty_qtree.generateProof(opened_xtag);

  proof.qualification.verdict = false;
  proof.qualification.witnesses.clear();
  proof.qualification.witnesses.push_back(negative_witness);

  response.anchor = initial_anchor;
  token.anchor = initial_anchor;
  response.result_slots.clear();

  const VerificationResult result =
      client.decryptAndVerify(response, token, token_request);
  EXPECT_TRUE(result.accepted);
  EXPECT_TRUE(result.ids.empty());
}

TEST_F(VQNomosTest, LargeScale1000Updates) {
  // Paper: VQNomos verifiable scheme (Section 4).
  // 200 docs × 5 keyword tiers = 1000 insertions.
  // QTree capacity 2^18 = 262144 keeps collision rate <2% for ~3000 xtag
  // writes; the proof-semantics fix handles any residual collisions correctly.
  const size_t qtree_cap = static_cast<size_t>(1) << 18;

  Gatekeeper gatekeeper;
  ASSERT_EQ(gatekeeper.setup(10, qtree_cap), 0);
  const Anchor initial_anchor = gatekeeper.getCurrentAnchor();
  Client client;
  ASSERT_EQ(
      client.setup(gatekeeper.getPublicKeyPem(), initial_anchor, qtree_cap, 10),
      0);
  Server server;
  server.setup(gatekeeper.getKm(), initial_anchor, qtree_cap);

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

  // 2-keyword intersection: {"tier_0", "global"} → 40 docs (i%5==0)
  {
    const std::vector<std::string> query = {"tier_0", "global"};
    const TokenRequest token_request =
        client.genToken(query, gatekeeper.getUpdateCounts());
    const SearchToken token = gatekeeper.genToken(token_request);
    const SearchRequest request = client.prepareSearch(token, token_request);
    const SearchResponse response = server.search(request, token);
    const VerificationResult result =
        client.decryptAndVerify(response, token, token_request);
    ASSERT_TRUE(result.accepted);
    EXPECT_EQ(result.ids.size(), 40u);
  }

  // 3-keyword intersection: {"niche_0","tier_0","global"} → 10 docs (i%20==0)
  {
    const std::vector<std::string> query = {"niche_0", "tier_0", "global"};
    const TokenRequest token_request =
        client.genToken(query, gatekeeper.getUpdateCounts());
    const SearchToken token = gatekeeper.genToken(token_request);
    const SearchRequest request = client.prepareSearch(token, token_request);
    const SearchResponse response = server.search(request, token);
    const VerificationResult result =
        client.decryptAndVerify(response, token, token_request);
    ASSERT_TRUE(result.accepted);
    EXPECT_EQ(result.ids.size(), 10u);
  }
}
