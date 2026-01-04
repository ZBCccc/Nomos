#pragma once

#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

extern "C" {
#include <relic/relic.h>
}

namespace core::crypto {

// Attribute set generic wrapper
using AttributeSet = std::set<std::string>;

// In this CP-ABE variants:
// Key is associated with AttributeSet S_A
// Ciphertext is associated with AttributeSet W (Policy is AND of all W)

struct MasterKey {
  bn_t alpha;
  bn_t a;
  ep2_t h_alpha;

  MasterKey() {
    bn_new(alpha);
    bn_new(a);
    ep2_new(h_alpha);
  }
  ~MasterKey() {
    bn_free(alpha);
    bn_free(a);
    ep2_free(h_alpha);
  }
  MasterKey(const MasterKey&) = delete;
  MasterKey& operator=(const MasterKey&) = delete;
};

struct PublicKey {
  ep_t g;
  ep2_t h;
  ep2_t h_a;  // h^a
  ep_t g_a;   // g^a
  gt_t e_gh_alpha;

  PublicKey() {
    ep_new(g);
    ep2_new(h);
    ep2_new(h_a);
    ep_new(g_a);
    gt_new(e_gh_alpha);
  }
  ~PublicKey() {
    ep_free(g);
    ep2_free(h);
    ep2_free(h_a);
    ep_free(g_a);
    gt_free(e_gh_alpha);
  }
  PublicKey(const PublicKey&) = delete;
  PublicKey& operator=(const PublicKey&) = delete;
};

struct SecretKey {
  ep2_t K;  // h^alpha * h^{at}
  ep2_t L;  // h^t

  // K_x = H(x)^t for each x in S_A
  struct Component {
    std::string attribute;
    ep2_t K_x;
    Component() { ep2_new(K_x); }
    ~Component() { ep2_free(K_x); }
    Component(Component&&) = default;  // Move only
  };
  std::vector<std::unique_ptr<Component>> components;
  AttributeSet attributes;  // S_A

  SecretKey() {
    ep2_new(K);
    ep2_new(L);
  }
  ~SecretKey() {
    ep2_free(K);
    ep2_free(L);
  }
  SecretKey(const SecretKey&) = delete;
  SecretKey& operator=(const SecretKey&) = delete;
};

struct Ciphertext {
  AttributeSet policy;  // The attribute set W
  gt_t C;               // M * e(g,h)^alpha*s
  ep_t C_prime;         // g^s

  // For AND policy of W:
  // We use simple n-out-of-n sharing of s.
  // s = sum(s_i)
  // Components for each w_i in W:
  // C_{1,i} = g^{a s_i} H(w_i)^{-r_i}
  // C_{2,i} = g^{r_i}

  struct Component {
    std::string attribute;
    ep_t C_1;
    ep_t C_2;
    Component() {
      ep_new(C_1);
      ep_new(C_2);
    }
    ~Component() {
      ep_free(C_1);
      ep_free(C_2);
    }
    Component(Component&&) = default;
  };
  std::vector<std::unique_ptr<Component>> components;

  Ciphertext() {
    gt_new(C);
    ep_new(C_prime);
  }
  ~Ciphertext() {
    gt_free(C);
    ep_free(C_prime);
  }
  Ciphertext(const Ciphertext&) = delete;
  Ciphertext& operator=(const Ciphertext&) = delete;
};

class CpABE {
 public:
  static void globalSetup();

  static void setup(MasterKey& mk, PublicKey& pk);

  // Key is associated with User Attribute Set
  static void keygen(SecretKey& sk, const MasterKey& mk, const PublicKey& pk,
                     const AttributeSet& userAttrs);

  // Encrypt under policy "AND of all attributes in policyAttrs"
  static bool encrypt(Ciphertext& ct, const PublicKey& pk,
                      const AttributeSet& policyAttrs, const gt_t& message);

  static bool decrypt(gt_t& recovered_message, const Ciphertext& ct,
                      const SecretKey& sk);
};

}  // namespace core::crypto
