#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "core/Primitive.hpp"
#include "mc-odxt/McOdxtTypes.hpp"

extern "C" {
#include <openssl/rand.h>
#include <openssl/sha.h>
}

namespace mcodxt {
namespace {

std::string serializePoint(const ep_t point) {
  uint8_t bytes[256];
  const int len = ep_size_bin(point, 1);
  ep_write_bin(bytes, len, point, 1);
  return std::string(reinterpret_cast<char*>(bytes), len);
}

void deserializePoint(ep_t point, const std::string& data) {
  ep_read_bin(point, reinterpret_cast<const uint8_t*>(data.data()),
              data.length());
}

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

  const std::string kz = F(serializePoint(hw), "1");
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

  uint8_t mask_bytes[256];
  const int mask_len = ep_size_bin(mask_point, 1);
  ep_write_bin(mask_bytes, mask_len, mask_point, 1);

  std::stringstream ss_plain;
  ss_plain << id << "|" << static_cast<int>(op);
  const std::string plaintext = ss_plain.str();

  const size_t enc_len =
      std::min(plaintext.length(), static_cast<size_t>(mask_len));
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

  // MC-ODXT: only one xtag per (keyword, id) pair (no redundancy).
  // xtag = H(w)^{Kx[I(w)] · F_p(Ky, id||op)}

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
  meta.xtag = serializePoint(xtag);

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

SearchToken McOdxtGatekeeper::genTokenSimplified(
    const std::vector<std::string>& query_keywords) {
  SearchToken token;
  const int n = static_cast<int>(query_keywords.size());
  if (n == 0) {
    return token;
  }

  const std::string& w1 = query_keywords[0];
  const int m = getUpdateCount(w1);
  if (m == 0) {
    return token;
  }

  ep_new(token.strap);
  ep_t hw1;
  ep_new(hw1);
  Hash_H1(hw1, w1);
  ep_mul(token.strap, hw1, m_Ks);
  ep_free(hw1);

  const int i1 = indexFunction(w1);
  token.bstag.clear();
  for (int j = 1; j <= m; ++j) {
    std::stringstream ss;
    ss << w1 << "|" << j << "|0";
    ep_t bstag;
    ep_new(bstag);
    Hash_H1(bstag, ss.str());
    ep_mul(bstag, bstag, m_Kt[i1]);
    token.bstag.push_back(serializePoint(bstag));
    ep_free(bstag);
  }

  token.delta.clear();
  for (int j = 1; j <= m; ++j) {
    std::stringstream ss;
    ss << w1 << "|" << j << "|1";
    ep_t delta;
    ep_new(delta);
    Hash_H1(delta, ss.str());
    ep_mul(delta, delta, m_Kt[i1]);
    token.delta.push_back(serializePoint(delta));
    ep_free(delta);
  }

  // MC-ODXT: single xtag (no redundancy), so k=1 and beta is fixed to 1.
  const int k = 1;

  token.bxtrap.clear();
  for (int i = 1; i < n; ++i) {
    const std::string& wi = query_keywords[i];
    const int idx = indexFunction(wi);

    ep_t hwi;
    ep_new(hwi);
    Hash_H1(hwi, wi);

    ep_t xtrap_i;
    ep_new(xtrap_i);
    ep_mul(xtrap_i, hwi, m_Kx[idx]);

    std::vector<std::string> bxtrap_i;
    // Single token: bxtrap = xtrap^1 = xtrap
    bxtrap_i.push_back(serializePoint(xtrap_i));

    token.bxtrap.push_back(bxtrap_i);
    ep_free(xtrap_i);
    ep_free(hwi);
  }

  return token;
}

McOdxtClient::McOdxtClient() {}

McOdxtClient::~McOdxtClient() {}

int McOdxtClient::setup() { return 0; }

SearchToken McOdxtClient::genTokenSimplified(
    const std::vector<std::string>& query_keywords,
    McOdxtGatekeeper& gatekeeper) {
  return gatekeeper.genTokenSimplified(query_keywords);
}

McOdxtClient::SearchRequest McOdxtClient::prepareSearch(
    const SearchToken& token, const std::vector<std::string>& query_keywords,
    const std::unordered_map<std::string, int>& updateCnt) {
  SearchRequest req;
  (void)updateCnt;
  const int n = static_cast<int>(query_keywords.size());
  if (n == 0) {
    return req;
  }

  const int m =
      static_cast<int>(std::min(token.bstag.size(), token.delta.size()));
  if (m == 0) {
    return req;
  }

  req.num_keywords = n;

  const std::string& w1 = query_keywords[0];
  const std::string kz = F(serializePoint(token.strap), "1");

  bn_t* e = new bn_t[m];
  for (int j = 1; j <= m; ++j) {
    bn_new(e[j - 1]);
    std::stringstream ss;
    ss << w1 << "|" << j;
    F_p(e[j - 1], kz, ss.str());
  }

  req.stokenList.clear();
  for (int j = 0; j < m; ++j) {
    req.stokenList.push_back(token.bstag[j]);
  }

  const int cross_keyword_count =
      std::min(static_cast<int>(token.bxtrap.size()), n - 1);
  req.xtokenList.clear();
  for (int j = 0; j < m; ++j) {
    std::vector<std::vector<std::string>> xtoken_list_j;
    for (int i = 0; i < cross_keyword_count; ++i) {
      std::vector<std::string> xtoken_list_ji;
      const int k = static_cast<int>(token.bxtrap[i].size());
      for (int t = 0; t < k; ++t) {
        ep_t bxtrap_it;
        ep_new(bxtrap_it);
        deserializePoint(bxtrap_it, token.bxtrap[i][t]);

        ep_t xtoken;
        ep_new(xtoken);
        ep_mul(xtoken, bxtrap_it, e[j]);

        xtoken_list_ji.push_back(serializePoint(xtoken));

        ep_free(xtoken);
        ep_free(bxtrap_it);
      }
      xtoken_list_j.push_back(xtoken_list_ji);
    }
    req.xtokenList.push_back(xtoken_list_j);
  }

  for (int j = 0; j < m; ++j) {
    bn_free(e[j]);
  }
  delete[] e;
  return req;
}

std::vector<std::string> McOdxtClient::decryptResults(
    const std::vector<SearchResultEntry>& results, const SearchToken& token) {
  // Paper: Algorithm 4 - Search (Section 4.3)
  // Backward privacy: net ADD - DEL per id; only return ids with count > 0.
  std::unordered_map<std::string, int> net_count;

  for (size_t i = 0; i < results.size(); ++i) {
    const SearchResultEntry& result = results[i];
    const int j = result.j;
    if (j < 1 || j > static_cast<int>(token.delta.size())) {
      continue;
    }

    ep_t delta_j;
    ep_new(delta_j);
    deserializePoint(delta_j, token.delta[j - 1]);

    uint8_t delta_bytes[256];
    const int delta_len = ep_size_bin(delta_j, 1);
    ep_write_bin(delta_bytes, delta_len, delta_j, 1);

    const size_t dec_len =
        std::min(result.sval.size(), static_cast<size_t>(delta_len));
    std::vector<uint8_t> plaintext(dec_len);
    for (size_t k = 0; k < dec_len; ++k) {
      plaintext[k] = result.sval[k] ^ delta_bytes[k];
    }

    ep_free(delta_j);

    const std::string decrypted(plaintext.begin(), plaintext.end());
    const size_t pos = decrypted.find('|');
    if (pos == std::string::npos) {
      continue;
    }

    const std::string id = decrypted.substr(0, pos);
    const int op = std::stoi(decrypted.substr(pos + 1));

    if (net_count.find(id) == net_count.end()) {
      net_count[id] = 0;
    }
    if (op == static_cast<int>(OpType::ADD)) {
      net_count[id]++;
    } else {
      net_count[id]--;
    }
  }

  std::vector<std::string> ids;
  for (std::unordered_map<std::string, int>::const_iterator it =
           net_count.begin();
       it != net_count.end(); ++it) {
    if (it->second > 0) {
      ids.push_back(it->first);
    }
  }

  return ids;
}

McOdxtServer::McOdxtServer() {}

McOdxtServer::~McOdxtServer() {
  m_TSet.clear();
  m_XSet.clear();
}

void McOdxtServer::setup(const std::vector<uint8_t>& /*Km*/) {}

std::string McOdxtServer::serializePoint(const ep_t point) const {
  uint8_t bytes[256];
  const int len = ep_size_bin(point, 1);
  ep_write_bin(bytes, len, point, 1);
  return std::string(reinterpret_cast<char*>(bytes), len);
}

void McOdxtServer::update(const UpdateMetadata& meta) {
  const std::string addr_key = serializePoint(meta.addr);

  TSetEntry entry;
  entry.val = meta.val;
  bn_new(entry.alpha);
  bn_copy(entry.alpha, meta.alpha);
  m_TSet[addr_key] = std::move(entry);

  if (!meta.xtag.empty()) {
    m_XSet[meta.xtag] = true;
  }
}

std::vector<SearchResultEntry> McOdxtServer::search(
    const McOdxtClient::SearchRequest& req) {
  std::vector<SearchResultEntry> results;

  const int m = static_cast<int>(req.stokenList.size());
  const int n = req.num_keywords;

  for (int j = 0; j < m; ++j) {
    const std::string& stag_key = req.stokenList[j];
    std::map<std::string, TSetEntry>::const_iterator it = m_TSet.find(stag_key);
    if (it == m_TSet.end()) {
      continue;
    }

    const TSetEntry& entry = it->second;
    bool all_match = true;
    int match_count = 0;

    if (n > 1) {
      for (int i = 0; i < n - 1; ++i) {
        if (j >= static_cast<int>(req.xtokenList.size()) ||
            i >= static_cast<int>(req.xtokenList[j].size())) {
          all_match = false;
          break;
        }

        const std::vector<std::string>& xtokens = req.xtokenList[j][i];
        bool keyword_match = false;

        for (size_t t = 0; t < xtokens.size(); ++t) {
          ep_t xtoken;
          ep_new(xtoken);
          ep_read_bin(xtoken,
                      reinterpret_cast<const uint8_t*>(xtokens[t].data()),
                      xtokens[t].length());

          ep_t xtag;
          ep_new(xtag);
          ep_mul(xtag, xtoken, entry.alpha);

          const std::string xtag_key = serializePoint(xtag);
          if (m_XSet.find(xtag_key) != m_XSet.end()) {
            keyword_match = true;
            match_count++;
          }

          ep_free(xtag);
          ep_free(xtoken);

          if (keyword_match) {
            break;
          }
        }

        if (!keyword_match) {
          all_match = false;
          break;
        }
      }
    }

    if (all_match) {
      SearchResultEntry result;
      result.j = j + 1;
      result.sval = entry.val;
      result.cnt = match_count;
      results.push_back(result);
    }
  }

  return results;
}

}  // namespace mcodxt
