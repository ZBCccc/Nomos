#include "vq-nomos/Gatekeeper.hpp"

#include <sstream>
#include <stdexcept>
#include <vector>

#include "core/Primitive.hpp"
#include "vq-nomos/Common.hpp"
#include "vq-nomos/MerkleOpen.hpp"

extern "C" {
#include <openssl/rand.h>
#include <openssl/sha.h>
}

namespace vqnomos {

namespace {

int sampleBetaIndex(int ell) {
  if (ell <= 0) {
    throw std::runtime_error("ell must be positive");
  }

  const uint8_t limit = static_cast<uint8_t>(256 - (256 % ell));
  uint8_t sample = 0;
  do {
    if (RAND_bytes(&sample, sizeof(sample)) != 1) {
      throw std::runtime_error("RAND_bytes failed while sampling beta");
    }
  } while (sample >= limit);

  return (sample % ell) + 1;
}

}  // namespace

Gatekeeper::Gatekeeper()
    : m_Kt(NULL),
      m_Kx(NULL),
      m_d(0),
      m_qtree(new QTree(1024)),
      m_signing_key(NULL) {
  bn_null(m_Ks);
  bn_null(m_Ky);
}

Gatekeeper::~Gatekeeper() {
  bn_free(m_Ks);
  bn_free(m_Ky);

  if (m_Kt != NULL) {
    for (int i = 0; i < m_d; ++i) {
      bn_free(m_Kt[i]);
    }
    delete[] m_Kt;
  }

  if (m_Kx != NULL) {
    for (int i = 0; i < m_d; ++i) {
      bn_free(m_Kx[i]);
    }
    delete[] m_Kx;
  }

  if (m_signing_key != NULL) {
    EVP_PKEY_free(m_signing_key);
  }
}

int Gatekeeper::setup(int d, size_t qtree_capacity) {
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

  m_qtree.reset(new QTree(qtree_capacity));
  m_qtree->initialize(std::vector<bool>(m_qtree->getCapacity(), false));

  if (m_signing_key != NULL) {
    EVP_PKEY_free(m_signing_key);
    m_signing_key = NULL;
  }
  EVP_PKEY_CTX* keygen_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
  if (keygen_ctx == NULL) {
    throw std::runtime_error("EVP_PKEY_CTX_new_id failed for Ed25519");
  }
  if (EVP_PKEY_keygen_init(keygen_ctx) != 1 ||
      EVP_PKEY_keygen(keygen_ctx, &m_signing_key) != 1) {
    EVP_PKEY_CTX_free(keygen_ctx);
    throw std::runtime_error("Ed25519 key generation failed");
  }
  EVP_PKEY_CTX_free(keygen_ctx);

  return 0;
}

UpdateMetadata Gatekeeper::update(OP op, const std::string& id,
                                  const std::string& keyword) {
  // Paper: Algorithm 2 + Chapter 3 Update'
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
  if (plaintext.length() > static_cast<size_t>(mask_len)) {
    ep_free(mask_point);
    throw std::runtime_error("Plaintext exceeds mask length");
  }

  meta.val.resize(plaintext.length());
  for (size_t i = 0; i < plaintext.length(); ++i) {
    meta.val[i] = plaintext[i] ^ mask_bytes[i];
  }
  ep_free(mask_point);

  bn_new(meta.alpha);
  bn_t fp_ky;
  bn_new(fp_ky);
  std::stringstream ss_id_op;
  ss_id_op << id << "|" << static_cast<int>(op);
  F_p(fp_ky, m_Ky, ss_id_op.str());

  bn_t fp_kz;
  bn_new(fp_kz);
  std::stringstream ss_w_cnt;
  ss_w_cnt << keyword << "|" << cnt;
  F_p(fp_kz, kz, ss_w_cnt.str());

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

  meta.xtags.clear();
  ep_t hw;
  ep_new(hw);
  Hash_H1(hw, keyword);

  bn_t fp_ky_id_op;
  bn_new(fp_ky_id_op);
  F_p(fp_ky_id_op, m_Ky, ss_id_op.str());

  bn_t ord2;
  bn_new(ord2);
  ep_curve_get_ord(ord2);

  const int ell = 3;
  for (int i = 1; i <= ell; ++i) {
    bn_t exp;
    bn_t i_bn;
    bn_new(exp);
    bn_new(i_bn);
    bn_set_dig(i_bn, i);

    bn_mul(exp, m_Kx[idx], fp_ky_id_op);
    bn_mul(exp, exp, i_bn);
    bn_mod(exp, exp, ord2);

    ep_t xtag;
    ep_new(xtag);
    ep_mul(xtag, hw, exp);
    meta.xtags.push_back(SerializePoint(xtag));

    ep_free(xtag);
    bn_free(exp);
    bn_free(i_bn);
  }

  ep_free(hw);
  bn_free(fp_ky_id_op);
  bn_free(ord2);

  meta.keyword = keyword;
  MerkleOpenTree merkle_tree(meta.xtags);
  meta.merkle_root = merkle_tree.getRootHash();
  meta.merkle_signature = signMerkleRoot(keyword, meta.merkle_root);

  m_qtree->updateBits(meta.xtags, true);
  meta.anchor = buildAnchor();
  return meta;
}

int Gatekeeper::getUpdateCount(const std::string& keyword) const {
  std::unordered_map<std::string, int>::const_iterator it =
      m_updateCnt.find(keyword);
  if (it == m_updateCnt.end()) {
    return 0;
  }
  return it->second;
}

SearchToken Gatekeeper::genToken(const TokenRequest& request) {
  // Paper: Algorithm 4 + TokenBind without OPRF blinding.
  SearchToken token;
  const int n = static_cast<int>(request.query_keywords.size());
  token.anchor = buildAnchor();

  if (n == 0 || request.hashed_keywords.empty()) {
    return token;
  }

  if (request.hashed_keywords.size() != request.query_keywords.size()) {
    throw std::invalid_argument("TokenRequest keyword/hash count mismatch");
  }
  if (request.hw1_j_0.size() != request.hw1_j_1.size()) {
    throw std::invalid_argument("TokenRequest stag/delta count mismatch");
  }

  const std::string& w1 = request.query_keywords[0];
  const int m = static_cast<int>(request.hw1_j_0.size());
  if (m == 0) {
    return token;
  }

  ep_new(token.strap);
  DeserializePoint(token.strap, request.hashed_keywords[0]);
  ep_mul(token.strap, token.strap, m_Ks);

  const int I1 = indexFunction(w1);
  for (int j = 0; j < m; ++j) {
    ep_t point;
    ep_new(point);
    DeserializePoint(point, request.hw1_j_0[static_cast<size_t>(j)]);
    ep_mul(point, point, m_Kt[I1]);
    token.bstag.push_back(SerializePoint(point));
    ep_free(point);
  }

  for (int j = 0; j < m; ++j) {
    ep_t point;
    ep_new(point);
    DeserializePoint(point, request.hw1_j_1[static_cast<size_t>(j)]);
    ep_mul(point, point, m_Kt[I1]);
    token.delta.push_back(SerializePoint(point));
    ep_free(point);
  }

  const int k = 2;
  const int ell = 3;
  std::vector<int> beta(k);
  for (int i = 0; i < k; ++i) {
    beta[i] = sampleBetaIndex(ell);
  }
  token.beta_indices = beta;

  for (int j = 1; j < n; ++j) {
    const std::string& wj = request.query_keywords[static_cast<size_t>(j)];
    const int Ij = indexFunction(wj);

    ep_t xtrap_j;
    ep_new(xtrap_j);
    DeserializePoint(xtrap_j, request.hashed_keywords[static_cast<size_t>(j)]);
    ep_mul(xtrap_j, xtrap_j, m_Kx[Ij]);

    std::vector<std::string> bxtrap_j;
    for (int t = 0; t < k; ++t) {
      bn_t beta_bn;
      ep_t bxtrap_jt;
      bn_new(beta_bn);
      bn_set_dig(beta_bn, beta[t]);
      ep_new(bxtrap_jt);
      ep_mul(bxtrap_jt, xtrap_j, beta_bn);
      bxtrap_j.push_back(SerializePoint(bxtrap_jt));
      ep_free(bxtrap_jt);
      bn_free(beta_bn);
    }
    token.bxtrap.push_back(bxtrap_j);
    ep_free(xtrap_j);
  }

  return token;
}

Anchor Gatekeeper::getCurrentAnchor() const { return buildAnchor(); }

std::string Gatekeeper::getPublicKeyPem() const {
  if (m_signing_key == NULL) {
    throw std::runtime_error("signing key is not initialized");
  }
  return ExportPublicKeyPem(m_signing_key);
}

int Gatekeeper::indexFunction(const std::string& keyword) const {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(keyword.c_str()),
         keyword.length(), hash);
  uint32_t index = 0;
  for (int i = 0; i < 4; ++i) {
    index = (index << 8) | hash[i];
  }
  return index % m_d;
}

std::string Gatekeeper::computeKz(const std::string& keyword) {
  ep_t hw;
  ep_new(hw);
  Hash_H1(hw, keyword);
  ep_mul(hw, hw, m_Ks);
  const std::string kz = F(SerializePoint(hw), "1");
  ep_free(hw);
  return kz;
}

std::string Gatekeeper::signMerkleRoot(const std::string& keyword,
                                       const std::string& root_hash) const {
  const int bucket_index = ComputeKeywordBucketIndex(keyword, m_d);
  return SignMessage(m_signing_key,
                     BuildMerkleAuthMessage(bucket_index, root_hash));
}

Anchor Gatekeeper::buildAnchor() const {
  Anchor anchor;
  anchor.version = m_qtree->getVersion();
  anchor.root_hash = m_qtree->getRootHash();
  anchor.signature = SignMessage(
      m_signing_key, BuildAnchorMessage(anchor.version, anchor.root_hash));
  return anchor;
}

}  // namespace vqnomos
