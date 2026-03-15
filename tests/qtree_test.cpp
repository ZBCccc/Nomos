#include "verifiable/QTree.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

extern "C" {
#include <relic/relic.h>
}

using namespace verifiable;

class QTreeTest : public ::testing::Test {
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
    // in this process. Cleaning here breaks subsequent suites.
  }
};

TEST_F(QTreeTest, CapacityRoundsUpToPowerOfTwo) {
  QTree tree5(5);
  EXPECT_EQ(tree5.getCapacity(), 8u);

  QTree tree8(8);
  EXPECT_EQ(tree8.getCapacity(), 8u);

  QTree tree1(1);
  EXPECT_EQ(tree1.getCapacity(), 1u);
}

TEST_F(QTreeTest, InitializeProducesNonEmptyRootHash) {
  QTree tree(4);
  tree.initialize({true, false, true, false});
  EXPECT_FALSE(tree.getRootHash().empty());
}

TEST_F(QTreeTest, InitializeSetsVersionToOne) {
  QTree tree(4);
  tree.initialize({false, false, false, false});
  EXPECT_EQ(tree.getVersion(), 1u);
}

TEST_F(QTreeTest, InitializeWithDifferentBitArraysProduceDifferentRoots) {
  QTree tree_a(4);
  tree_a.initialize({true, false, false, false});

  QTree tree_b(4);
  tree_b.initialize({false, true, false, false});

  EXPECT_NE(tree_a.getRootHash(), tree_b.getRootHash());
}

TEST_F(QTreeTest, InitializeIsDeterministic) {
  QTree tree_a(4);
  tree_a.initialize({true, false, true, true});

  QTree tree_b(4);
  tree_b.initialize({true, false, true, true});

  EXPECT_EQ(tree_a.getRootHash(), tree_b.getRootHash());
}

TEST_F(QTreeTest, InitializeSmallBitArrayPadsWithZeros) {
  QTree tree_a(4);
  tree_a.initialize({true, false});

  QTree tree_b(4);
  tree_b.initialize({true, false, false, false});

  EXPECT_EQ(tree_a.getRootHash(), tree_b.getRootHash());
}

TEST_F(QTreeTest, UninitializedTreeHasEmptyRootHash) {
  QTree tree(4);
  EXPECT_TRUE(tree.getRootHash().empty());
}

TEST_F(QTreeTest, UninitializedVersionIsZero) {
  QTree tree(4);
  EXPECT_EQ(tree.getVersion(), 0u);
}

TEST_F(QTreeTest, UpdateBitChangesRootHash) {
  // Paper: QTree update (Chapter 2)
  QTree tree(4);
  tree.initialize({false, false, false, false});
  const std::string root_before = tree.getRootHash();

  tree.updateBit("addr_a", true);

  EXPECT_NE(tree.getRootHash(), root_before);
}

TEST_F(QTreeTest, UpdateBitIncrementsVersion) {
  QTree tree(4);
  tree.initialize({false, false, false, false});
  EXPECT_EQ(tree.getVersion(), 1u);

  tree.updateBit("x", true);
  EXPECT_EQ(tree.getVersion(), 2u);

  tree.updateBit("y", false);
  EXPECT_EQ(tree.getVersion(), 3u);
}

TEST_F(QTreeTest, UpdateBitRoundTripRestoresRoot) {
  QTree tree(4);
  tree.initialize({false, false, false, false});
  const std::string root_original = tree.getRootHash();

  const std::string addr = "flip_addr";
  tree.updateBit(addr, true);
  EXPECT_NE(tree.getRootHash(), root_original);

  tree.updateBit(addr, false);
  EXPECT_EQ(tree.getRootHash(), root_original);
}

TEST_F(QTreeTest, VerifyPathSucceedsForUpdatedBit) {
  // Paper: QTree proof verification (Chapter 2)
  QTree tree(8);
  tree.initialize({false, false, false, false, false, false, false, false});

  const std::string addr = "test_address";
  tree.updateBit(addr, true);

  const std::vector<std::string> proof = tree.generateProof(addr);
  EXPECT_TRUE(tree.verifyPath(addr, true, proof, tree.getRootHash()));
}

TEST_F(QTreeTest, VerifyPathFailsWithWrongBitValue) {
  QTree tree(8);
  tree.initialize({false, false, false, false, false, false, false, false});

  const std::string addr = "addr_wrong_bit";
  tree.updateBit(addr, true);

  const std::vector<std::string> proof = tree.generateProof(addr);
  EXPECT_FALSE(tree.verifyPath(addr, false, proof, tree.getRootHash()));
}

TEST_F(QTreeTest, VerifyPathFailsWithStaleRoot) {
  QTree tree(8);
  tree.initialize({false, false, false, false, false, false, false, false});

  const std::string addr = "addr_stale";
  tree.updateBit(addr, true);
  const std::string stale_root = tree.getRootHash();

  tree.updateBit("addr_other", true);
  ASSERT_NE(stale_root, tree.getRootHash());

  const std::vector<std::string> proof = tree.generateProof(addr);
  EXPECT_FALSE(tree.verifyPath(addr, true, proof, stale_root));
}

TEST_F(QTreeTest, VerifyPathSucceedsForZeroBit) {
  QTree tree(4);
  tree.initialize({true, true, true, true});

  const std::string addr = "addr_zero";
  tree.updateBit(addr, false);

  const std::vector<std::string> proof = tree.generateProof(addr);
  EXPECT_TRUE(tree.verifyPath(addr, false, proof, tree.getRootHash()));
}

TEST_F(QTreeTest, ProofLengthEqualsLogCapacity) {
  QTree tree(8);
  tree.initialize({});

  const std::vector<std::string> proof = tree.generateProof("some_addr");
  EXPECT_EQ(proof.size(), 3u);
}

TEST_F(QTreeTest, PositiveProofContainsExpectedHeader) {
  QTree tree(4);
  tree.initialize({true, true, false, false});

  const std::string proof =
      tree.generatePositiveProof({"addr_pos_1", "addr_pos_2"});
  EXPECT_EQ(proof.substr(0, 8), "POSITIVE");
}

TEST_F(QTreeTest, NegativeProofContainsExpectedHeader) {
  QTree tree(4);
  tree.initialize({false, false, false, false});

  const std::string proof = tree.generateNegativeProof("addr_neg");
  EXPECT_EQ(proof.substr(0, 8), "NEGATIVE");
}

TEST_F(QTreeTest, PositiveProofDiffersForDifferentAddressSets) {
  QTree tree(8);
  tree.initialize({});

  const std::string proof_a = tree.generatePositiveProof({"addr1", "addr2"});
  const std::string proof_b = tree.generatePositiveProof({"addr3", "addr4"});

  EXPECT_NE(proof_a, proof_b);
}
