#include <gtest/gtest.h>
#include <iostream>
#include "core/CpABE.hpp"

using namespace core::crypto;

class CpABETest : public ::testing::Test {
protected:
    void SetUp() override {
        CpABE::globalSetup();
        CpABE::setup(mk, pk);
    }

    MasterKey mk;
    PublicKey pk;
};

// Test 1: Subset Match (User has MORE than needed)
TEST_F(CpABETest, SubsetMatch) {
    AttributeSet userAttrs = {"AttrA", "AttrB", "AttrC"};
    AttributeSet queryAttrs = {"AttrA", "AttrB"}; // Query needs A AND B

    SecretKey sk;
    CpABE::keygen(sk, mk, pk, userAttrs);

    gt_t msg, recovered;
    gt_new(msg); gt_new(recovered);
    gt_rand(msg);

    Ciphertext ct;
    ASSERT_TRUE(CpABE::encrypt(ct, pk, queryAttrs, msg));

    ASSERT_TRUE(CpABE::decrypt(recovered, ct, sk));
    ASSERT_EQ(gt_cmp(msg, recovered), CMP_EQ);

    gt_free(msg); gt_free(recovered);
}

// Test 2: Exact Match
TEST_F(CpABETest, ExactMatch) {
    AttributeSet userAttrs = {"AttrA", "AttrB"};
    AttributeSet queryAttrs = {"AttrA", "AttrB"};

    SecretKey sk;
    CpABE::keygen(sk, mk, pk, userAttrs);

    gt_t msg, recovered;
    gt_new(msg); gt_new(recovered);
    gt_rand(msg);

    Ciphertext ct;
    ASSERT_TRUE(CpABE::encrypt(ct, pk, queryAttrs, msg));

    ASSERT_TRUE(CpABE::decrypt(recovered, ct, sk));
    ASSERT_EQ(gt_cmp(msg, recovered), CMP_EQ);

    gt_free(msg); gt_free(recovered);
}

// Test 3: Superset Fail (User has LESS than needed)
TEST_F(CpABETest, SupersetFail) {
    AttributeSet userAttrs = {"AttrA", "AttrB"};
    AttributeSet queryAttrs = {"AttrA", "AttrB", "AttrC"}; // Needs C

    SecretKey sk;
    CpABE::keygen(sk, mk, pk, userAttrs);

    gt_t msg, recovered;
    gt_new(msg); gt_new(recovered);
    gt_rand(msg);

    Ciphertext ct;
    ASSERT_TRUE(CpABE::encrypt(ct, pk, queryAttrs, msg));

    // Decrypt should fail (returns false)
    ASSERT_FALSE(CpABE::decrypt(recovered, ct, sk));

    gt_free(msg); gt_free(recovered);
}

// Test 4: Disjoint Fail
TEST_F(CpABETest, DisjointFail) {
    AttributeSet userAttrs = {"AttrA"};
    AttributeSet queryAttrs = {"AttrB"};

    SecretKey sk;
    CpABE::keygen(sk, mk, pk, userAttrs);

    gt_t msg, recovered;
    gt_new(msg); gt_new(recovered);
    gt_rand(msg);

    Ciphertext ct;
    ASSERT_TRUE(CpABE::encrypt(ct, pk, queryAttrs, msg));

    ASSERT_FALSE(CpABE::decrypt(recovered, ct, sk));

    gt_free(msg); gt_free(recovered);
}
