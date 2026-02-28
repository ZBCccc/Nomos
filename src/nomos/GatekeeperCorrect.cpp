#include "nomos/GatekeeperCorrect.hpp"

#include <sstream>

#include "core/Primitive.hpp"

extern "C" {
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
}

namespace nomos {

// Helper function to serialize elliptic curve point to string
static std::string serializePoint(const ep_t point) {
  uint8_t bytes[256];
  int len = ep_size_bin(point, 1);
  ep_write_bin(bytes, len, point, 1);
  return std::string(reinterpret_cast<char*>(bytes), len);
}

GatekeeperCorrect::GatekeeperCorrect() : m_d(0), m_Kt(nullptr), m_Kx(nullptr) {
  bn_null(m_Ks);
  bn_null(m_Ky);
}

GatekeeperCorrect::~GatekeeperCorrect() {
  bn_free(m_Ks);
  bn_free(m_Ky);

  if (m_Kt != nullptr) {
    for (int i = 0; i < m_d; ++i) {
      bn_free(m_Kt[i]);
    }
    delete[] m_Kt;
  }

  if (m_Kx != nullptr) {
    for (int i = 0; i < m_d; ++i) {
      bn_free(m_Kx[i]);
    }
    delete[] m_Kx;
  }
}

int GatekeeperCorrect::setup(int d) {
  m_d = d;

  // Get curve order
  bn_t ord;
  bn_new(ord);
  ep_curve_get_ord(ord);

  // Sample Ks from Zp*
  bn_new(m_Ks);
  bn_rand_mod(m_Ks, ord);

  // Sample Kt[1..d] from Zp*
  m_Kt = new bn_t[d];
  for (int i = 0; i < d; ++i) {
    bn_new(m_Kt[i]);
    bn_rand_mod(m_Kt[i], ord);
  }

  // Sample Kx[1..d] from Zp*
  m_Kx = new bn_t[d];
  for (int i = 0; i < d; ++i) {
    bn_new(m_Kx[i]);
    bn_rand_mod(m_Kx[i], ord);
  }

  // Sample Ky from {0,1}^λ (as bn_t for Fp)
  bn_new(m_Ky);
  bn_rand_mod(m_Ky, ord);

  // Sample Km from {0,1}^λ (for AE)
  m_Km.resize(32);  // 256 bits
  RAND_bytes(m_Km.data(), 32);

  // Initialize UpdateCnt
  m_updateCnt.clear();

  bn_free(ord);
  return 0;
}

int GatekeeperCorrect::indexFunction(const std::string& keyword) const {
  // I(w): hash keyword to index in [0, d-1]
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(keyword.c_str()),
         keyword.length(), hash);
  uint32_t index = 0;
  for (int i = 0; i < 4; ++i) {
    index = (index << 8) | hash[i];
  }
  return index % m_d;
}

void GatekeeperCorrect::computeKz(bn_t kz, const std::string& keyword) {
  // Kz = F((H(w))^Ks, 1)
  // Step 1: Compute H(w)
  ep_t hw;
  ep_new(hw);
  Hash_H1(hw, keyword);

  // Step 2: Compute (H(w))^Ks
  ep_mul(hw, hw, m_Ks);

  // Step 3: Serialize to bytes
  uint8_t hw_bytes[256];
  int hw_len = ep_size_bin(hw, 1);
  ep_write_bin(hw_bytes, hw_len, hw, 1);

  // Step 4: F(hw_bytes, 1) -> hash to Zp
  std::string input(reinterpret_cast<char*>(hw_bytes), hw_len);
  input += "|1";
  Hash_Zn(kz, input);

  ep_free(hw);
}

void GatekeeperCorrect::computeFp(bn_t result, bn_t key,
                                  const std::string& input) {
  // Fp(key, input): PRF from Zp* x {0,1}* -> Zp*
  // Serialize key
  uint8_t key_bytes[256];
  int key_len = bn_size_bin(key);
  bn_write_bin(key_bytes, key_len, key);

  // Concatenate key and input
  std::string combined(reinterpret_cast<char*>(key_bytes), key_len);
  combined += "|" + input;

  // Hash to Zp
  Hash_Zn(result, combined);
}

UpdateMetadata GatekeeperCorrect::update(OP op, const std::string& id,
                                         const std::string& keyword) {
  UpdateMetadata meta;

  // Step 1: Compute Kz = F((H(w))^Ks, 1)
  bn_t kz;
  bn_new(kz);
  computeKz(kz, keyword);

  // Step 2: Update counter
  if (m_updateCnt.find(keyword) == m_updateCnt.end()) {
    m_updateCnt[keyword] = 0;
  }
  m_updateCnt[keyword]++;
  int cnt = m_updateCnt[keyword];

  // Step 3: Compute addr = (H(w||cnt||0))^Kt[I(w)]
  int idx = indexFunction(keyword);
  std::stringstream ss_addr;
  ss_addr << keyword << "|" << cnt << "|0";

  ep_new(meta.addr);
  Hash_H1(meta.addr, ss_addr.str());
  ep_mul(meta.addr, meta.addr, m_Kt[idx]);

  // Step 4: Compute val = (id||op) ⊕ (H(w||cnt||1))^Kt[I(w)]
  std::stringstream ss_mask;
  ss_mask << keyword << "|" << cnt << "|1";

  ep_t mask_point;
  ep_new(mask_point);
  Hash_H1(mask_point, ss_mask.str());
  ep_mul(mask_point, mask_point, m_Kt[idx]);

  // Serialize mask
  uint8_t mask_bytes[256];
  int mask_len = ep_size_bin(mask_point, 1);
  ep_write_bin(mask_bytes, mask_len, mask_point, 1);

  // Prepare plaintext
  std::stringstream ss_plain;
  ss_plain << id << "|" << static_cast<int>(op);
  std::string plaintext = ss_plain.str();

  // XOR encryption
  meta.val.resize(plaintext.length());
  for (size_t i = 0; i < plaintext.length(); ++i) {
    meta.val[i] = plaintext[i] ^ mask_bytes[i % mask_len];
  }

  ep_free(mask_point);

  // Step 5: Compute alpha = Fp(Ky, id||op) · (Fp(Kz, w||cnt))^{-1}
  bn_new(meta.alpha);

  bn_t fp_ky;
  bn_new(fp_ky);
  std::stringstream ss_id_op;
  ss_id_op << id << "|" << static_cast<int>(op);
  computeFp(fp_ky, m_Ky, ss_id_op.str());

  bn_t fp_kz;
  bn_new(fp_kz);
  std::stringstream ss_w_cnt;
  ss_w_cnt << keyword << "|" << cnt;
  computeFp(fp_kz, kz, ss_w_cnt.str());

  // Compute inverse
  bn_t ord;
  bn_new(ord);
  ep_curve_get_ord(ord);

  bn_t fp_kz_inv;
  bn_new(fp_kz_inv);
  bn_mod_inv(fp_kz_inv, fp_kz, ord);

  // alpha = fp_ky * fp_kz_inv mod ord
  bn_mul(meta.alpha, fp_ky, fp_kz_inv);
  bn_mod(meta.alpha, meta.alpha, ord);

  bn_free(fp_ky);
  bn_free(fp_kz);
  bn_free(fp_kz_inv);
  bn_free(ord);

  // Step 6: Compute xtag_i = H(w)^{Kx[I(w)] · Fp(Ky, id||op) · i}
  const int ell = 3;  // Parameter ℓ
  meta.xtags.clear();

  ep_t hw;
  ep_new(hw);
  Hash_H1(hw, keyword);

  bn_t fp_ky_id_op;
  bn_new(fp_ky_id_op);
  computeFp(fp_ky_id_op, m_Ky, ss_id_op.str());

  bn_t ord2;
  bn_new(ord2);
  ep_curve_get_ord(ord2);

  for (int i = 1; i <= ell; ++i) {
    // Compute exponent: Kx[I(w)] · Fp(Ky, id||op) · i
    bn_t exp;
    bn_new(exp);

    bn_mul(exp, m_Kx[idx], fp_ky_id_op);
    bn_t i_bn;
    bn_new(i_bn);
    bn_set_dig(i_bn, i);
    bn_mul(exp, exp, i_bn);
    bn_mod(exp, exp, ord2);

    // Compute xtag_i = H(w)^exp
    ep_t xtag;
    ep_new(xtag);
    ep_mul(xtag, hw, exp);

    // Serialize and store
    meta.xtags.push_back(serializePoint(xtag));

    ep_free(xtag);
    bn_free(exp);
    bn_free(i_bn);
  }

  ep_free(hw);
  bn_free(fp_ky_id_op);
  bn_free(ord2);
  bn_free(kz);

  return meta;
}

int GatekeeperCorrect::getUpdateCount(const std::string& keyword) const {
  auto it = m_updateCnt.find(keyword);
  if (it == m_updateCnt.end()) {
    return 0;
  }
  return it->second;
}

/*
// FULL OPRF VERSION - Not used in simplified implementation
GatekeeperCorrect::BlindedTokens GatekeeperCorrect::genTokenGatekeeper(
    const std::vector<std::string>& query_keywords,
    const std::vector<ep_t>& blinded_a,
    const std::vector<ep_t>& blinded_b,
    const std::vector<ep_t>& blinded_c,
    const std::vector<int>& av) {

    BlindedTokens tokens;
    int n = query_keywords.size();
    int m = getUpdateCount(query_keywords[0]);  // w1 is primary keyword

    // Check access control (simplified - always allow for now)
    // if (av not in P) abort

    bn_t ord;
    bn_new(ord);
    ep_curve_get_ord(ord);

    // Sample rho_1, ..., rho_n
    std::vector<bn_t> rho(n);
    for (int i = 0; i < n; ++i) {
        bn_new(rho[i]);
        bn_rand_mod(rho[i], ord);
    }

    // Sample gamma_1, ..., gamma_m
    std::vector<bn_t> gamma(m);
    for (int j = 0; j < m; ++j) {
        bn_new(gamma[j]);
        bn_rand_mod(gamma[j], ord);
    }

    // Compute strap' = (a_1)^Ks
    ep_new(tokens.strap_prime);
    ep_mul(tokens.strap_prime, blinded_a[0], m_Ks);

    // Compute bstag'_j = (b_j)^{Kt[I1] · gamma_j}
    int I1 = av[0];
    tokens.bstag_prime.resize(m);
    for (int j = 0; j < m; ++j) {
        bn_t exp;
        bn_new(exp);
        bn_mul(exp, m_Kt[I1], gamma[j]);
        bn_mod(exp, exp, ord);

        ep_new(tokens.bstag_prime[j]);
        ep_mul(tokens.bstag_prime[j], blinded_b[j], exp);

        bn_free(exp);
    }

    // Compute delta'_j = (c_j)^{Kt[I1]}
    tokens.delta_prime.resize(m);
    for (int j = 0; j < m; ++j) {
        ep_new(tokens.delta_prime[j]);
        ep_mul(tokens.delta_prime[j], blinded_c[j], m_Kt[I1]);
    }

    // Compute bxtrap'_j = (a_j)^{Kx[Ij] · rho_j} for j=2..n
    std::vector<ep_t> bxtrap_prime(n - 1);
    for (int j = 1; j < n; ++j) {
        int Ij = av[j];
        bn_t exp;
        bn_new(exp);
        bn_mul(exp, m_Kx[Ij], rho[j]);
        bn_mod(exp, exp, ord);

        ep_new(bxtrap_prime[j - 1]);
        ep_mul(bxtrap_prime[j - 1], blinded_a[j], exp);

        bn_free(exp);
    }

    // Sample RBF random indices beta_i in [ell]
    const int k = 2;  // Parameter k
    const int ell = 3;  // Parameter ℓ
    std::vector<int> beta(k);
    for (int i = 0; i < k; ++i) {
        beta[i] = (rand() % ell) + 1;  // Random in [1, ℓ]
    }

    // For j=2..n, compute overline{bxtrap}'_j
    tokens.bxtrap_prime.resize(n - 1);
    for (int j = 1; j < n; ++j) {
        tokens.bxtrap_prime[j - 1].resize(k);
        for (int t = 0; t < k; ++t) {
            bn_t beta_bn;
            bn_new(beta_bn);
            bn_set_dig(beta_bn, beta[t]);

            ep_new(tokens.bxtrap_prime[j - 1][t]);
            ep_mul(tokens.bxtrap_prime[j - 1][t], bxtrap_prime[j - 1], beta_bn);

            bn_free(beta_bn);
        }
    }

    // Clean up bxtrap_prime
    for (auto& bx : bxtrap_prime) {
        ep_free(bx);
    }

    // Encrypt env = AE.Enc_Km(rho_1, ..., rho_n, gamma_1, ..., gamma_m)
    // Simplified: serialize rho and gamma
    std::vector<uint8_t> plaintext_env;
    for (int i = 0; i < n; ++i) {
        uint8_t rho_bytes[256];
        int rho_len = bn_size_bin(rho[i]);
        bn_write_bin(rho_bytes, rho_len, rho[i]);
        plaintext_env.insert(plaintext_env.end(), rho_bytes, rho_bytes +
rho_len);
    }
    for (int j = 0; j < m; ++j) {
        uint8_t gamma_bytes[256];
        int gamma_len = bn_size_bin(gamma[j]);
        bn_write_bin(gamma_bytes, gamma_len, gamma[j]);
        plaintext_env.insert(plaintext_env.end(), gamma_bytes, gamma_bytes +
gamma_len);
    }

    // Simple XOR encryption with Km (should use proper AE)
    tokens.env.resize(plaintext_env.size());
    for (size_t i = 0; i < plaintext_env.size(); ++i) {
        tokens.env[i] = plaintext_env[i] ^ m_Km[i % m_Km.size()];
    }

    // Clean up
    for (auto& r : rho) bn_free(r);
    for (auto& g : gamma) bn_free(g);
    bn_free(ord);

    return tokens;
}
*/

SearchToken GatekeeperCorrect::genTokenSimplified(
    const std::vector<std::string>& query_keywords) {
  SearchToken token;
  int n = query_keywords.size();
  if (n == 0) return token;

  const std::string& w1 = query_keywords[0];
  int m = getUpdateCount(w1);
  if (m == 0) return token;

  bn_t ord;
  bn_new(ord);
  ep_curve_get_ord(ord);

  // Step 1: Compute strap = H(w1)^Ks
  ep_new(token.strap);
  ep_t hw1;
  ep_new(hw1);
  Hash_H1(hw1, w1);
  ep_mul(token.strap, hw1, m_Ks);
  ep_free(hw1);

  // Step 2: Compute stag_j = H(w1||j||0)^Kt[I(w1)] for j=1..m
  int I1 = indexFunction(w1);
  token.bstag.clear();
  for (int j = 1; j <= m; ++j) {
    std::stringstream ss;
    ss << w1 << "|" << j << "|0";
    ep_t bstag;
    ep_new(bstag);
    Hash_H1(bstag, ss.str());
    ep_mul(bstag, bstag, m_Kt[I1]);
    token.bstag.push_back(serializePoint(bstag));
    ep_free(bstag);
  }

  // Step 3: Compute delta_j = H(w1||j||1)^Kt[I(w1)] for j=1..m
  token.delta.clear();
  for (int j = 1; j <= m; ++j) {
    std::stringstream ss;
    ss << w1 << "|" << j << "|1";
    ep_t delta;
    ep_new(delta);
    Hash_H1(delta, ss.str());
    ep_mul(delta, delta, m_Kt[I1]);
    token.delta.push_back(serializePoint(delta));
    ep_free(delta);
  }

  // Step 4 & 5: Compute xtrap_j = H(wj)^Kx[I(wj)] and bxtrap
  const int k = 2;    // Parameter k
  const int ell = 3;  // Parameter ℓ
  std::vector<int> beta(k);
  for (int i = 0; i < k; ++i) {
    beta[i] = (rand() % ell) + 1;  // Random in [1, ℓ]
  }

  token.bxtrap.clear();
  for (int j = 1; j < n; ++j) {
    const std::string& wj = query_keywords[j];
    int Ij = indexFunction(wj);

    // Compute xtrap_j = H(wj)^Kx[Ij]
    ep_t xtrap_j;
    ep_new(xtrap_j);
    ep_t hwj;
    ep_new(hwj);
    Hash_H1(hwj, wj);
    ep_mul(xtrap_j, hwj, m_Kx[Ij]);
    ep_free(hwj);

    // Compute bxtrap_j[t] = xtrap_j^beta[t]
    std::vector<std::string> bxtrap_j;
    for (int t = 0; t < k; ++t) {
      bn_t beta_bn;
      bn_new(beta_bn);
      bn_set_dig(beta_bn, beta[t]);

      ep_t bxtrap_jt;
      ep_new(bxtrap_jt);
      ep_mul(bxtrap_jt, xtrap_j, beta_bn);
      bxtrap_j.push_back(serializePoint(bxtrap_jt));

      ep_free(bxtrap_jt);
      bn_free(beta_bn);
    }
    token.bxtrap.push_back(bxtrap_j);

    ep_free(xtrap_j);
  }

  // Step 6: Sample rho and gamma, encrypt env
  bn_t* rho = new bn_t[n];
  for (int i = 0; i < n; ++i) {
    bn_new(rho[i]);
    bn_rand_mod(rho[i], ord);
  }

  bn_t* gamma = new bn_t[m];
  for (int j = 0; j < m; ++j) {
    bn_new(gamma[j]);
    bn_rand_mod(gamma[j], ord);
  }

  // Serialize rho and gamma
  std::vector<uint8_t> plaintext_env;
  for (int i = 0; i < n; ++i) {
    uint8_t rho_bytes[256];
    int rho_len = bn_size_bin(rho[i]);
    bn_write_bin(rho_bytes, rho_len, rho[i]);
    plaintext_env.insert(plaintext_env.end(), rho_bytes, rho_bytes + rho_len);
  }
  for (int j = 0; j < m; ++j) {
    uint8_t gamma_bytes[256];
    int gamma_len = bn_size_bin(gamma[j]);
    bn_write_bin(gamma_bytes, gamma_len, gamma[j]);
    plaintext_env.insert(plaintext_env.end(), gamma_bytes,
                         gamma_bytes + gamma_len);
  }

  // XOR encryption with Km
  token.env.resize(plaintext_env.size());
  for (size_t i = 0; i < plaintext_env.size(); ++i) {
    token.env[i] = plaintext_env[i] ^ m_Km[i % m_Km.size()];
  }

  // Clean up
  for (int i = 0; i < n; ++i) bn_free(rho[i]);
  for (int j = 0; j < m; ++j) bn_free(gamma[j]);
  delete[] rho;
  delete[] gamma;
  bn_free(ord);

  return token;
}

}  // namespace nomos
