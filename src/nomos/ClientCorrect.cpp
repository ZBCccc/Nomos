#include "nomos/ClientCorrect.hpp"

#include <iostream>
#include <sstream>

#include "core/Primitive.hpp"

extern "C" {
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

// Helper function to deserialize string to elliptic curve point
static void deserializePoint(ep_t point, const std::string& str) {
  ep_read_bin(point, reinterpret_cast<const uint8_t*>(str.data()),
              str.length());
}

ClientCorrect::ClientCorrect() : m_r(nullptr), m_s(nullptr), m_n(0), m_m(0) {}

ClientCorrect::~ClientCorrect() {
  // Clean up blinding factors
  freeBlindingFactors();
}

int ClientCorrect::setup() { return 0; }

SearchToken ClientCorrect::genTokenSimplified(
    const std::vector<std::string>& query_keywords,
    GatekeeperCorrect& gatekeeper) {
  // Delegate to gatekeeper's simplified token generation
  return gatekeeper.genTokenSimplified(query_keywords);
}

ClientCorrect::SearchRequest ClientCorrect::prepareSearch(
    const SearchToken& token, const std::vector<std::string>& query_keywords,
    const std::unordered_map<std::string, int>& updateCnt) {
  SearchRequest req;
  int n = query_keywords.size();
  if (n == 0) return req;

  const std::string& w1 = query_keywords[0];
  auto it = updateCnt.find(w1);
  if (it == updateCnt.end()) return req;
  int m = it->second;

  // Get curve order
  bn_t ord;
  bn_new(ord);
  ep_curve_get_ord(ord);

  // Step 1: Compute Kz = F(strap, 1)
  // Serialize strap
  uint8_t strap_bytes[256];
  int strap_len = ep_size_bin(token.strap, 1);
  ep_write_bin(strap_bytes, strap_len, token.strap, 1);

  std::string strap_str(reinterpret_cast<char*>(strap_bytes), strap_len);
  strap_str += "|1";

  bn_t kz;
  bn_new(kz);
  Hash_Zn(kz, strap_str);

  // Step 2: For each j=1..m, compute e_j = Fp(Kz, w1||j)
  bn_t* e = new bn_t[m];
  for (int j = 1; j <= m; ++j) {
    bn_new(e[j - 1]);

    // Serialize Kz
    uint8_t kz_bytes[256];
    int kz_len = bn_size_bin(kz);
    bn_write_bin(kz_bytes, kz_len, kz);

    // Concatenate Kz and w1||j
    std::stringstream ss;
    ss << w1 << "|" << j;
    std::string combined(reinterpret_cast<char*>(kz_bytes), kz_len);
    combined += "|" + ss.str();

    // Hash to Zp
    Hash_Zn(e[j - 1], combined);
  }

  // Step 3: Compute stoken_j = bstag_j^{1/gamma_j}
  // For simplified version, we don't have gamma, so we use bstag directly
  req.stokenList.clear();
  for (int j = 0; j < m; ++j) {
    req.stokenList.push_back(token.bstag[j]);
  }

  // Step 4: Compute xtoken[i][j][t] = bxtrap[j][t]^{e_j}
  const int k = 2;  // Parameter k
  req.xtokenList.clear();
  for (int j = 0; j < m; ++j) {
    std::vector<std::vector<std::string>> xtokenList_j;
    for (int i = 0; i < n - 1; ++i) {
      std::vector<std::string> xtokenList_ji;
      for (int t = 0; t < k; ++t) {
        // Deserialize bxtrap[i][t]
        ep_t bxtrap_it;
        ep_new(bxtrap_it);
        deserializePoint(bxtrap_it, token.bxtrap[i][t]);

        // Compute xtoken = bxtrap^{e_j}
        ep_t xtoken;
        ep_new(xtoken);
        ep_mul(xtoken, bxtrap_it, e[j]);

        xtokenList_ji.push_back(serializePoint(xtoken));

        ep_free(xtoken);
        ep_free(bxtrap_it);
      }
      xtokenList_j.push_back(xtokenList_ji);
    }
    req.xtokenList.push_back(xtokenList_j);
  }

  // Step 5: Copy env
  req.env = token.env;

  // Clean up
  for (int j = 0; j < m; ++j) bn_free(e[j]);
  delete[] e;
  bn_free(kz);
  bn_free(ord);

  return req;
}

std::vector<std::string> ClientCorrect::decryptResults(
    const std::vector<SearchResultEntry>& results, const SearchToken& token) {
  std::vector<std::string> ids;

  for (const auto& result : results) {
    int j = result.j;
    if (j < 1 || j > static_cast<int>(token.delta.size())) {
      continue;  // Invalid index
    }

    // Decrypt sval using delta_j
    // sval = (id||op) ⊕ delta_j
    ep_t delta_j;
    ep_new(delta_j);
    deserializePoint(delta_j, token.delta[j - 1]);

    // Serialize delta_j
    uint8_t delta_bytes[256];
    int delta_len = ep_size_bin(delta_j, 1);
    ep_write_bin(delta_bytes, delta_len, delta_j, 1);

    // XOR decryption
    std::vector<uint8_t> plaintext(result.sval.size());
    for (size_t i = 0; i < result.sval.size(); ++i) {
      plaintext[i] = result.sval[i] ^ delta_bytes[i % delta_len];
    }

    // Parse (id||op)
    std::string decrypted(plaintext.begin(), plaintext.end());
    size_t pos = decrypted.find('|');
    if (pos == std::string::npos) {
      ep_free(delta_j);
      continue;  // Invalid format
    }

    std::string id = decrypted.substr(0, pos);
    int op = std::stoi(decrypted.substr(pos + 1));

    // Filter by operation (only return ADD operations)
    if (op == OP_ADD) {
      ids.push_back(id);
    }

    ep_free(delta_j);
  }

  return ids;
}

// Paper: Algorithm 3, Client side (lines 1-6)
// OPRF Phase 1: Generate blinded request
BlindedRequest ClientCorrect::genTokenPhase1(
    const std::vector<std::string>& query_keywords,
    const std::unordered_map<std::string, int>& updateCnt) {
  BlindedRequest request;

  m_n = query_keywords.size();
  if (m_n == 0) return request;

  const std::string& w1 = query_keywords[0];
  auto it = updateCnt.find(w1);
  if (it == updateCnt.end()) return request;
  m_m = it->second;

  bn_t ord;
  bn_new(ord);
  ep_curve_get_ord(ord);

  // Free old blinding factors if any (but save m_n, m_m first)
  int saved_n = m_n;
  int saved_m = m_m;
  freeBlindingFactors();
  m_n = saved_n;
  m_m = saved_m;

  // Allocate memory for blinding factors
  m_r = new bn_t[m_n];
  m_s = new bn_t[m_m];

  // Step 1: Sample r_1, ..., r_n from Zp* (Paper: line 2)
  for (int j = 0; j < m_n; ++j) {
    bn_new(m_r[j]);
    bn_rand_mod(m_r[j], ord);
  }

  // Step 2: Sample s_1, ..., s_m from Zp* (Paper: line 2)
  for (int j = 0; j < m_m; ++j) {
    bn_new(m_s[j]);
    bn_rand_mod(m_s[j], ord);
  }

  // Step 3: Compute a_j = H(w_j)^{r_j}, j=1..n (Paper: line 3)
  request.a.resize(m_n);
  for (int j = 0; j < m_n; ++j) {
    ep_t hw_j, a_j;
    ep_new(hw_j);
    ep_new(a_j);
    Hash_H1(hw_j, query_keywords[j]);
    ep_mul(a_j, hw_j, m_r[j]);
    request.a[j] = serializePoint(a_j);
    ep_free(hw_j);
    ep_free(a_j);
  }

  // Step 4: Compute b_j = H(w_1||j||0)^{s_j}, j=1..m (Paper: line 4)
  request.b.resize(m_m);
  for (int j = 1; j <= m_m; ++j) {
    std::stringstream ss;
    ss << w1 << "|" << j << "|0";
    ep_t hb_j, b_j;
    ep_new(hb_j);
    ep_new(b_j);
    Hash_H1(hb_j, ss.str());
    ep_mul(b_j, hb_j, m_s[j - 1]);
    request.b[j - 1] = serializePoint(b_j);
    ep_free(hb_j);
    ep_free(b_j);
  }

  // Step 5: Compute c_j = H(w_1||j||1)^{s_j}, j=1..m (Paper: line 5)
  request.c.resize(m_m);
  for (int j = 1; j <= m_m; ++j) {
    std::stringstream ss;
    ss << w1 << "|" << j << "|1";
    ep_t hc_j, c_j;
    ep_new(hc_j);
    ep_new(c_j);
    Hash_H1(hc_j, ss.str());
    ep_mul(c_j, hc_j, m_s[j - 1]);
    request.c[j - 1] = serializePoint(c_j);
    ep_free(hc_j);
    ep_free(c_j);
  }

  // Step 6: Compute access vector av = (I(w_1), ..., I(w_n)) (Paper: line 6)
  request.av.resize(m_n);
  for (int j = 0; j < m_n; ++j) {
    // I(w): hash keyword to index
    unsigned char hash[32];
    SHA256(reinterpret_cast<const unsigned char*>(query_keywords[j].c_str()),
           query_keywords[j].length(), hash);
    uint32_t index = 0;
    for (int i = 0; i < 4; ++i) {
      index = (index << 8) | hash[i];
    }
    request.av[j] = index % 10;  // Assuming d=10
  }

  bn_free(ord);
  return request;
}

// Paper: Algorithm 3, Client side (lines 15-19)
// OPRF Phase 2: Unblind response to get final token
SearchToken ClientCorrect::genTokenPhase2(const BlindedResponse& response) {
  SearchToken token;

  if (m_n == 0 || m_m == 0 || m_r == nullptr || m_s == nullptr) {
    return token;  // Phase 1 not called
  }

  bn_t ord;
  bn_new(ord);
  ep_curve_get_ord(ord);

  // Step 1: Compute strap = (strap')^{r_1^{-1}} (Paper: line 15)
  bn_t r1_inv;
  bn_new(r1_inv);
  bn_mod_inv(r1_inv, m_r[0], ord);

  ep_t strap_prime;
  ep_new(strap_prime);
  ep_new(token.strap);
  deserializePoint(strap_prime, response.strap_prime);
  ep_mul(token.strap, strap_prime, r1_inv);
  ep_free(strap_prime);
  bn_free(r1_inv);

  // Step 2: Compute bstag_j = (bstag'_j)^{s_j^{-1}}, j=1..m (Paper: line 16)
  token.bstag.resize(m_m);
  for (int j = 0; j < m_m; ++j) {
    bn_t sj_inv;
    bn_new(sj_inv);
    bn_mod_inv(sj_inv, m_s[j], ord);

    ep_t bstag_prime_j, bstag_j;
    ep_new(bstag_prime_j);
    ep_new(bstag_j);
    deserializePoint(bstag_prime_j, response.bstag_prime[j]);
    ep_mul(bstag_j, bstag_prime_j, sj_inv);
    token.bstag[j] = serializePoint(bstag_j);

    ep_free(bstag_prime_j);
    ep_free(bstag_j);
    bn_free(sj_inv);
  }

  // Step 3: Compute delta_j = (delta'_j)^{s_j^{-1}}, j=1..m (Paper: line 17)
  token.delta.resize(m_m);
  for (int j = 0; j < m_m; ++j) {
    bn_t sj_inv;
    bn_new(sj_inv);
    bn_mod_inv(sj_inv, m_s[j], ord);

    ep_t delta_prime_j, delta_j;
    ep_new(delta_prime_j);
    ep_new(delta_j);
    deserializePoint(delta_prime_j, response.delta_prime[j]);
    ep_mul(delta_j, delta_prime_j, sj_inv);
    token.delta[j] = serializePoint(delta_j);

    ep_free(delta_prime_j);
    ep_free(delta_j);
    bn_free(sj_inv);
  }

  // Step 4: Compute bxtrap_j = (bxtrap'_j)^{r_j^{-1}}, j=2..n (Paper: lines
  // 18-19)
  token.bxtrap.resize(m_n - 1);
  for (int j = 1; j < m_n; ++j) {
    bn_t rj_inv;
    bn_new(rj_inv);
    bn_mod_inv(rj_inv, m_r[j], ord);

    int k = response.bxtrap_prime[j - 1].size();
    token.bxtrap[j - 1].resize(k);
    for (int t = 0; t < k; ++t) {
      ep_t bxtrap_prime_jt, bxtrap_jt;
      ep_new(bxtrap_prime_jt);
      ep_new(bxtrap_jt);
      deserializePoint(bxtrap_prime_jt, response.bxtrap_prime[j - 1][t]);
      ep_mul(bxtrap_jt, bxtrap_prime_jt, rj_inv);
      token.bxtrap[j - 1][t] = serializePoint(bxtrap_jt);

      ep_free(bxtrap_prime_jt);
      ep_free(bxtrap_jt);
    }

    bn_free(rj_inv);
  }

  // Step 5: Copy env (Paper: line 19)
  token.env = response.env;

  // Clean up blinding factors
  freeBlindingFactors();

  bn_free(ord);
  return token;
}

void ClientCorrect::freeBlindingFactors() {
  if (m_r != nullptr) {
    for (int j = 0; j < m_n; ++j) {
      bn_free(m_r[j]);
    }
    delete[] m_r;
    m_r = nullptr;
  }
  if (m_s != nullptr) {
    for (int j = 0; j < m_m; ++j) {
      bn_free(m_s[j]);
    }
    delete[] m_s;
    m_s = nullptr;
  }
  m_n = 0;
  m_m = 0;
}

}  // namespace nomos
