#include <gtest/gtest.h>

#include "core/Primitive.hpp"

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
