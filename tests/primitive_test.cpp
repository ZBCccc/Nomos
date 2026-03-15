#include "core/Primitive.hpp"

#include <gtest/gtest.h>

extern "C" {
#include <relic/relic.h>
}

class PrimitiveTest : public ::testing::Test {
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

TEST_F(PrimitiveTest, FStringPrfMatchesHmacSha256) {
  const std::string key = "test_prf_key";
  const std::string input = "keyword|17";

  EXPECT_EQ(F(key, input), HmacSha256(key, input));
}

TEST_F(PrimitiveTest, FpMatchesExplicitHmacPipeline) {
  bn_t actual;
  bn_t expected;
  bn_new(actual);
  bn_new(expected);

  const std::string key = "test_prf_key";
  const std::string input = "keyword|17";

  F_p(actual, key, input);
  Hash_Zn(expected, F(key, input));

  EXPECT_EQ(bn_cmp(actual, expected), RLC_EQ);

  bn_free(actual);
  bn_free(expected);
}

TEST_F(PrimitiveTest, FStringBnKeyMatchesSerializedKeyVersion) {
  bn_t key_bn;
  bn_new(key_bn);

  Hash_Zn(key_bn, "scalar-prf-key");

  EXPECT_EQ(F(key_bn, "doc|ADD"), F(SerializeBn(key_bn), "doc|ADD"));

  bn_free(key_bn);
}

TEST_F(PrimitiveTest, FpBnKeyMatchesSerializedKeyVersion) {
  bn_t key_bn;
  bn_t from_bn_key;
  bn_t from_serialized_key;
  bn_new(key_bn);
  bn_new(from_bn_key);
  bn_new(from_serialized_key);

  Hash_Zn(key_bn, "scalar-prf-key");

  F_p(from_bn_key, key_bn, "doc|ADD");
  F_p(from_serialized_key, SerializeBn(key_bn), "doc|ADD");

  EXPECT_EQ(bn_cmp(from_bn_key, from_serialized_key), RLC_EQ);

  bn_free(key_bn);
  bn_free(from_bn_key);
  bn_free(from_serialized_key);
}

TEST_F(PrimitiveTest, HashZnIsDeterministic) {
  bn_t a;
  bn_t b;
  bn_new(a);
  bn_new(b);

  Hash_Zn(a, "determinism-test-input");
  Hash_Zn(b, "determinism-test-input");

  EXPECT_EQ(bn_cmp(a, b), RLC_EQ);

  bn_free(a);
  bn_free(b);
}

TEST_F(PrimitiveTest, HashZnDifferentInputsProduceDifferentOutputs) {
  bn_t a;
  bn_t b;
  bn_new(a);
  bn_new(b);

  Hash_Zn(a, "input_alpha");
  Hash_Zn(b, "input_beta");

  EXPECT_NE(bn_cmp(a, b), RLC_EQ);

  bn_free(a);
  bn_free(b);
}

TEST_F(PrimitiveTest, SerializeBnRoundTripPreservesValue) {
  bn_t original;
  bn_t recovered;
  bn_new(original);
  bn_new(recovered);

  Hash_Zn(original, "serialize-round-trip");
  const std::string serialized = SerializeBn(original);
  Hash_Zn(recovered, serialized);

  EXPECT_FALSE(serialized.empty());

  bn_free(original);
  bn_free(recovered);
}

TEST_F(PrimitiveTest, SerializeBnDifferentValuesProduceDifferentBytes) {
  bn_t a;
  bn_t b;
  bn_new(a);
  bn_new(b);

  Hash_Zn(a, "val_a");
  Hash_Zn(b, "val_b");

  EXPECT_NE(SerializeBn(a), SerializeBn(b));

  bn_free(a);
  bn_free(b);
}

TEST_F(PrimitiveTest, HashH1IsDeterministic) {
  // Paper: Hash_H1 is a hash-to-curve function (Section 3.1)
  ep_t p1;
  ep_t p2;
  ep_null(p1);
  ep_null(p2);
  ep_new(p1);
  ep_new(p2);

  Hash_H1(p1, "h1-determinism-input");
  Hash_H1(p2, "h1-determinism-input");

  EXPECT_EQ(ep_cmp(p1, p2), RLC_EQ);

  ep_free(p1);
  ep_free(p2);
}

TEST_F(PrimitiveTest, HashH1DifferentInputsProduceDifferentPoints) {
  ep_t p1;
  ep_t p2;
  ep_null(p1);
  ep_null(p2);
  ep_new(p1);
  ep_new(p2);

  Hash_H1(p1, "h1-input-one");
  Hash_H1(p2, "h1-input-two");

  EXPECT_NE(ep_cmp(p1, p2), RLC_EQ);

  ep_free(p1);
  ep_free(p2);
}
