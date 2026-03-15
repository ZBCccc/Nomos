#include "nomos/Client.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "core/Primitive.hpp"

namespace nomos {

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

Client::Client() {}

Client::~Client() {}

int Client::setup() { return 0; }

TokenRequest Client::genToken(
    const std::vector<std::string>& query_keywords,
    const std::unordered_map<std::string, int>& updateCnt) {
  // Paper: Algorithm 4 - Nomos GenToken (Client side)
  // Simplified experiment path: keep query reordering and hashing on the
  // client, but omit the paper's OPRF blinding/deblinding.
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
    std::stringstream ss_b;
    std::stringstream ss_c;
    ss_b << w1 << "|" << j << "|0";
    ss_c << w1 << "|" << j << "|1";

    ep_t b;
    ep_t c;
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

Client::SearchRequest Client::prepareSearch(const SearchToken& token,
                                            const TokenRequest& token_request) {
  // Paper: Algorithm 5 - Nomos Search (Client side)
  SearchRequest req;
  int n = static_cast<int>(token_request.query_keywords.size());
  if (n == 0) return req;

  int m = static_cast<int>(std::min(token.bstag.size(), token.delta.size()));
  if (m == 0) return req;

  req.num_keywords = n;

  const std::string& w1 = token_request.query_keywords[0];

  // Step 1: Compute Kz = F(strap, 1)
  const std::string kz = F(SerializePoint(token.strap), "1");

  // Step 2: For each j=1..m, compute e_j = F_p(Kz, w1||j)
  bn_t* e = new bn_t[m];
  for (int j = 1; j <= m; ++j) {
    bn_new(e[j - 1]);

    // Apply the scalar-valued PRF to derive the per-position scalar e_j.
    std::stringstream ss;
    ss << w1 << "|" << j;
    F_p(e[j - 1], kz, ss.str());
  }

  // Step 3: Copy stokens from the simplified token.
  req.stokenList.clear();
  for (int j = 0; j < m; ++j) {
    req.stokenList.push_back(token.bstag[j]);
  }

  // Step 4: Compute xtoken[i][j][t] = bxtrap[j][t]^{e_j}
  const int k = 2;  // Parameter k
  const int crossKeywordCount =
      std::min(static_cast<int>(token.bxtrap.size()), n - 1);
  req.xtokenList.clear();
  for (int j = 0; j < m; ++j) {
    std::vector<std::vector<std::string>> xtokenList_j;
    for (int i = 0; i < crossKeywordCount; ++i) {
      std::vector<std::string> xtokenList_ji;
      for (int t = 0; t < k; ++t) {
        // Deserialize bxtrap[i][t]
        ep_t bxtrap_it;
        ep_new(bxtrap_it);
        DeserializePoint(bxtrap_it, token.bxtrap[i][t]);

        // Compute xtoken = bxtrap^{e_j}
        ep_t xtoken;
        ep_new(xtoken);
        ep_mul(xtoken, bxtrap_it, e[j]);

        xtokenList_ji.push_back(SerializePoint(xtoken));

        ep_free(xtoken);
        ep_free(bxtrap_it);
      }
      xtokenList_j.push_back(xtokenList_ji);
    }
    req.xtokenList.push_back(xtokenList_j);
  }

  // Clean up
  for (int j = 0; j < m; ++j) bn_free(e[j]);
  delete[] e;
  return req;
}

std::vector<std::string> Client::decryptResults(
    const std::vector<SearchResultEntry>& results, const SearchToken& token) {
  // Paper: Algorithm 4 - Search (Section 4.3)
  // Correct backward privacy: a document that was ADD-ed then DEL-eted must NOT
  // appear in results.  We tally net ADD count per id: count[id] = #ADDs -
  // #DELs across all TSet entries that decrypted successfully.  An id is live
  // iff count[id] > 0.
  std::unordered_map<std::string, int> net_count;

  for (const auto& result : results) {
    int j = result.j;
    if (j < 1 || j > static_cast<int>(token.delta.size())) {
      continue;  // Invalid index
    }

    // Decrypt sval using delta_j
    // sval = (id||op) ⊕ delta_j
    ep_t delta_j;
    ep_new(delta_j);
    DeserializePoint(delta_j, token.delta[j - 1]);

    // Serialize delta_j safely
    const int delta_len = ep_size_bin(delta_j, 1);
    if (delta_len <= 0) {
      ep_free(delta_j);
      continue;
    }
    std::vector<uint8_t> delta_bytes(static_cast<size_t>(delta_len));
    ep_write_bin(delta_bytes.data(), delta_len, delta_j, 1);

    // XOR decryption
    const size_t dec_len =
        std::min(result.sval.size(), static_cast<size_t>(delta_len));
    std::vector<uint8_t> plaintext(dec_len);
    for (size_t i = 0; i < dec_len; ++i) {
      plaintext[i] = result.sval[i] ^ delta_bytes[i];
    }

    ep_free(delta_j);

    // Parse (id||op)
    std::string decrypted(plaintext.begin(), plaintext.end());
    size_t pos = decrypted.find('|');
    if (pos == std::string::npos) {
      continue;  // Invalid format
    }

    std::string id = decrypted.substr(0, pos);
    int op = std::stoi(decrypted.substr(pos + 1));

    // Accumulate: ADD increments, DEL decrements
    if (net_count.find(id) == net_count.end()) {
      net_count[id] = 0;
    }
    if (op == OP_ADD) {
      net_count[id]++;
    } else {
      net_count[id]--;
    }
  }

  // Collect ids whose net count is positive (still live in the database)
  std::vector<std::string> ids;
  for (const auto& kv : net_count) {
    if (kv.second > 0) {
      ids.push_back(kv.first);
    }
  }

  return ids;
}

}  // namespace nomos
