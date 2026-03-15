#include "mc-odxt/McOdxtClient.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "core/Primitive.hpp"

namespace mcodxt {

namespace {

int getUpdateCount(const std::unordered_map<std::string, int>& updateCnt,
                   const std::string& keyword) {
  std::unordered_map<std::string, int>::const_iterator it =
      updateCnt.find(keyword);
  if (it == updateCnt.end()) {
    return 0;
  }
  return it->second;
}

}  // namespace

McOdxtClient::McOdxtClient() {}

McOdxtClient::~McOdxtClient() {}

int McOdxtClient::setup() { return 0; }

TokenRequest McOdxtClient::genToken(
    const std::vector<std::string>& query_keywords,
    const std::unordered_map<std::string, int>& updateCnt) {
  // Paper: Algorithm 4 - Nomos GenToken (Client side)
  // MC-ODXT simplification: keep the query reordering and point hashing on the
  // client, but omit OPRF blinding and RBF expansion.
  TokenRequest req;

  const int n = static_cast<int>(query_keywords.size());
  if (n == 0) {
    return req;
  }

  req.query_keywords = query_keywords;

  int min_update_count = getUpdateCount(updateCnt, req.query_keywords[0]);
  int min_index = 0;
  for (int i = 1; i < n; ++i) {
    const int current_count = getUpdateCount(updateCnt, req.query_keywords[i]);
    if (current_count < min_update_count) {
      min_update_count = current_count;
      min_index = i;
    }
  }

  if (min_index != 0) {
    std::swap(req.query_keywords[0], req.query_keywords[min_index]);
  }

  const std::string& w1 = req.query_keywords[0];
  const int m = getUpdateCount(updateCnt, w1);
  if (m == 0) {
    return req;
  }

  for (int i = 0; i < n; ++i) {
    ep_t hw;
    ep_new(hw);
    Hash_H1(hw, req.query_keywords[i]);
    req.hashed_keywords.push_back(SerializePoint(hw));
    ep_free(hw);
  }

  for (int j = 1; j <= m; ++j) {
    std::stringstream ss_b, ss_c;
    ss_b << w1 << "|" << j << "|0";
    ss_c << w1 << "|" << j << "|1";

    ep_t b, c;
    ep_new(b);
    ep_new(c);
    Hash_H1(b, ss_b.str());
    Hash_H1(c, ss_c.str());

    req.hw1_j_0.push_back(SerializePoint(b));
    req.hw1_j_1.push_back(SerializePoint(c));

    ep_free(b);
    ep_free(c);
  }

  return req;
}

McOdxtClient::SearchRequest McOdxtClient::prepareSearch(
    const SearchToken& token, const TokenRequest& token_request) {
  // Paper: Algorithm 5 - Nomos Search (Client side)
  // MC-ODXT simplification: each x-term carries a single bxtrap, so no RBF
  // sampling or tuple permutation is needed here.
  SearchRequest req;
  const int n = static_cast<int>(token_request.query_keywords.size());
  if (n == 0) {
    return req;
  }

  const int m =
      static_cast<int>(std::min(token.bstag.size(), token.delta.size()));
  if (m == 0) {
    return req;
  }

  req.num_keywords = n;

  const std::string& w1 = token_request.query_keywords[0];
  const std::string kz = F(SerializePoint(token.strap), "1");

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
        DeserializePoint(bxtrap_it, token.bxtrap[i][t]);

        ep_t xtoken;
        ep_new(xtoken);
        ep_mul(xtoken, bxtrap_it, e[j]);

        xtoken_list_ji.push_back(SerializePoint(xtoken));

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
  std::unordered_map<std::string, int> net_count;

  for (size_t i = 0; i < results.size(); ++i) {
    const SearchResultEntry& result = results[i];
    const int j = result.j;
    if (j < 1 || j > static_cast<int>(token.delta.size())) {
      continue;
    }

    ep_t delta_j;
    ep_new(delta_j);
    DeserializePoint(delta_j, token.delta[j - 1]);

    const int delta_len = ep_size_bin(delta_j, 1);
    if (delta_len <= 0) {
      ep_free(delta_j);
      continue;
    }
    std::vector<uint8_t> delta_bytes(static_cast<size_t>(delta_len));
    ep_write_bin(delta_bytes.data(), delta_len, delta_j, 1);

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

}  // namespace mcodxt
