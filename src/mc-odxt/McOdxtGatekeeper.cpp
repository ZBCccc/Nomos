#include "mc-odxt/McOdxtGatekeeper.hpp"

#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include "core/Primitive.hpp"

extern "C" {
#include <openssl/rand.h>
#include <openssl/sha.h>
}

namespace mcodxt {

McOdxtGatekeeper::McOdxtGatekeeper() : m_Kt(nullptr), m_Kx(nullptr), m_d(0) {
  bn_null(m_Ks);
  bn_null(m_Ky);
}

McOdxtGatekeeper::~McOdxtGatekeeper() {
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

int McOdxtGatekeeper::setup(int d) {
  m_d = d;

  bn_t ord;
  bn_new(ord);
  ep_curve_get_ord(ord);

  bn_new(m_Ks);
  bn_rand_mod(m_Ks, ord);

  m_Kt = new bn_t[d];
  for (int i = 0; i < d; ++i) {
    bn_new(m_Kt[i]);
    bn_rand_mod(m_Kt[i], ord);
  }

  m_Kx = new bn_t[d];
  for (int i = 0; i < d; ++i) {
    bn_new(m_Kx[i]);
    bn_rand_mod(m_Kx[i], ord);
  }

  bn_new(m_Ky);
  bn_rand_mod(m_Ky, ord);

  m_Km.resize(32);
  if (RAND_bytes(m_Km.data(), 32) != 1) {
    bn_free(ord);
    throw std::runtime_error("RAND_bytes failed while generating Km");
  }

  m_updateCnt.clear();

  bn_free(ord);
  return 0;
}

int McOdxtGatekeeper::indexFunction(const std::string& keyword) const {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(keyword.c_str()),
         keyword.length(), hash);

  uint32_t index = 0;
  for (int i = 0; i < 4; ++i) {
    index = (index << 8) | hash[i];
  }
  return index % m_d;
}

std::string McOdxtGatekeeper::computeKz(const std::string& keyword) {
  ep_t hw;
  ep_new(hw);
  Hash_H1(hw, keyword);
  ep_mul(hw, hw, m_Ks);

  const std::string kz = F(SerializePoint(hw), "1");
  ep_free(hw);
  return kz;
}

void McOdxtGatekeeper::computeF_p(bn_t result, const bn_t key,
                                  const std::string& input) {
  F_p(result, key, input);
}

void McOdxtGatekeeper::computeF_p(bn_t result, const std::string& key,
                                  const std::string& input) {
  F_p(result, key, input);
}

UpdateMetadata McOdxtGatekeeper::update(OpType op, const std::string& id,
                                        const std::string& keyword) {
  UpdateMetadata meta;

  const std::string kz = computeKz(keyword);

  if (m_updateCnt.find(keyword) == m_updateCnt.end()) {
    m_updateCnt[keyword] = 0;
  }
  m_updateCnt[keyword]++;
  const int cnt = m_updateCnt[keyword];
  const int idx = indexFunction(keyword);

  std::stringstream ss_addr;
  ss_addr << keyword << "|" << cnt << "|0";

  ep_new(meta.addr);
  Hash_H1(meta.addr, ss_addr.str());
  ep_mul(meta.addr, meta.addr, m_Kt[idx]);

  std::stringstream ss_mask;
  ss_mask << keyword << "|" << cnt << "|1";

  ep_t mask_point;
  ep_new(mask_point);
  Hash_H1(mask_point, ss_mask.str());
  ep_mul(mask_point, mask_point, m_Kt[idx]);

  const int mask_len = ep_size_bin(mask_point, 1);
  if (mask_len <= 0) {
    ep_free(mask_point);
    throw std::runtime_error("Invalid mask point size");
  }
  std::vector<uint8_t> mask_bytes(static_cast<size_t>(mask_len));
  ep_write_bin(mask_bytes.data(), mask_len, mask_point, 1);

  std::stringstream ss_plain;
  ss_plain << id << "|" << static_cast<int>(op);
  const std::string plaintext = ss_plain.str();

  // Truncation check: ensure mask is sufficient for the entire plaintext.
  if (plaintext.length() > static_cast<size_t>(mask_len)) {
    ep_free(mask_point);
    throw std::runtime_error(
        "Plaintext (id||op) exceeds mask length derived from elliptic curve "
        "point. ID too long?");
  }

  const size_t enc_len = plaintext.length();
  meta.val.resize(enc_len);
  for (size_t i = 0; i < enc_len; ++i) {
    meta.val[i] = plaintext[i] ^ mask_bytes[i];
  }
  ep_free(mask_point);

  bn_new(meta.alpha);

  bn_t fp_ky;
  bn_new(fp_ky);
  computeF_p(fp_ky, m_Ky, ss_plain.str());

  bn_t fp_kz;
  bn_new(fp_kz);
  std::stringstream ss_w_cnt;
  ss_w_cnt << keyword << "|" << cnt;
  computeF_p(fp_kz, kz, ss_w_cnt.str());

  bn_t ord;
  bn_new(ord);
  ep_curve_get_ord(ord);

  bn_t fp_kz_inv;
  bn_new(fp_kz_inv);
  bn_mod_inv(fp_kz_inv, fp_kz, ord);

  bn_mul(meta.alpha, fp_ky, fp_kz_inv);
  bn_mod(meta.alpha, meta.alpha, ord);

  bn_free(fp_ky);
  bn_free(fp_kz);
  bn_free(fp_kz_inv);
  bn_free(ord);

  ep_t hw;
  ep_new(hw);
  Hash_H1(hw, keyword);

  bn_t fp_ky_id_op;
  bn_new(fp_ky_id_op);
  computeF_p(fp_ky_id_op, m_Ky, ss_plain.str());

  bn_t ord2;
  bn_new(ord2);
  ep_curve_get_ord(ord2);

  bn_t exp;
  bn_new(exp);
  bn_mul(exp, m_Kx[idx], fp_ky_id_op);
  bn_mod(exp, exp, ord2);

  ep_t xtag;
  ep_new(xtag);
  ep_mul(xtag, hw, exp);
  meta.xtag = SerializePoint(xtag);

  ep_free(xtag);
  bn_free(exp);

  ep_free(hw);
  bn_free(fp_ky_id_op);
  bn_free(ord2);
  return meta;
}

int McOdxtGatekeeper::getUpdateCount(const std::string& keyword) const {
  std::unordered_map<std::string, int>::const_iterator it =
      m_updateCnt.find(keyword);
  if (it == m_updateCnt.end()) {
    return 0;
  }
  return it->second;
}

void McOdxtGatekeeper::setUpdateCountForBenchmark(const std::string& keyword,
                                                  int count) {
  m_updateCnt[keyword] = count;
}

SearchToken McOdxtGatekeeper::genToken(const TokenRequest& req) {
  // Paper: Algorithm 4 - Nomos GenToken (Gatekeeper side)
  // MC-ODXT simplification: apply Ks, Kt and Kx directly to the client-supplied
  // hash points and skip the paper's RBF expansion.

  SearchToken token;

  if (req.query_keywords.empty() || req.hashed_keywords.empty()) {
    return token;
  }

  if (req.hashed_keywords.size() != req.query_keywords.size()) {
    throw std::invalid_argument(
        "TokenRequest keyword/hash count mismatch in MC-ODXT genToken");
  }
  if (req.hw1_j_0.size() != req.hw1_j_1.size()) {
    throw std::invalid_argument(
        "TokenRequest stag/delta count mismatch in MC-ODXT genToken");
  }

  // Step 1: Compute strap = hw1^Ks
  ep_new(token.strap);
  DeserializePoint(token.strap, req.hashed_keywords[0]);
  ep_mul(token.strap, token.strap, m_Ks);

  // Step 2: Compute bstag_j = H(w1||j||0)^Kt[I(w1)] for j=1..m
  const std::string& w1 = req.query_keywords[0];
  const int i1 = indexFunction(w1);
  const int m = static_cast<int>(req.hw1_j_0.size());

  token.bstag.clear();
  token.delta.clear();

  for (int j = 0; j < m; ++j) {
    ep_t bstag;
    ep_new(bstag);
    DeserializePoint(bstag, req.hw1_j_0[j]);
    ep_mul(bstag, bstag, m_Kt[i1]);
    token.bstag.push_back(SerializePoint(bstag));
    ep_free(bstag);

    ep_t delta;
    ep_new(delta);
    DeserializePoint(delta, req.hw1_j_1[j]);
    ep_mul(delta, delta, m_Kt[i1]);
    token.delta.push_back(SerializePoint(delta));
    ep_free(delta);
  }

  // Step 3: Compute bxtrap for cross-keywords
  token.bxtrap.clear();
  const int num_other = static_cast<int>(req.hashed_keywords.size()) - 1;

  for (int i = 0; i < num_other; ++i) {
    const int idx = indexFunction(req.query_keywords[i + 1]);

    ep_t xtrap;
    ep_new(xtrap);
    DeserializePoint(xtrap, req.hashed_keywords[i + 1]);
    ep_mul(xtrap, xtrap, m_Kx[idx]);

    std::vector<std::string> bxtrap_i;
    bxtrap_i.push_back(SerializePoint(xtrap));
    token.bxtrap.push_back(bxtrap_i);

    ep_free(xtrap);
  }

  return token;
}

}  // namespace mcodxt
