#include "nomos/Client.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "core/Primitive.hpp"

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

Client::Client() {}

Client::~Client() {}

int Client::setup() { return 0; }

SearchToken Client::genTokenSimplified(
    const std::vector<std::string>& query_keywords, Gatekeeper& gatekeeper) {
  // Delegate token generation to the single simplified path on the Gatekeeper side.
  return gatekeeper.genTokenSimplified(query_keywords);
}

Client::SearchRequest Client::prepareSearch(
    const SearchToken& token, const std::vector<std::string>& query_keywords,
    const std::unordered_map<std::string, int>& updateCnt) {
  SearchRequest req;
  (void)updateCnt;
  int n = query_keywords.size();
  if (n == 0) return req;

  int m = std::min(token.bstag.size(), token.delta.size());
  if (m == 0) return req;

  const std::string& w1 = query_keywords[0];

  // Step 1: Compute Kz = F(strap, 1)
  const std::string kz = F(serializePoint(token.strap), "1");

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
  const int crossKeywordCount = std::min(static_cast<int>(token.bxtrap.size()), n - 1);
  req.xtokenList.clear();
  for (int j = 0; j < m; ++j) {
    std::vector<std::vector<std::string>> xtokenList_j;
    for (int i = 0; i < crossKeywordCount; ++i) {
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

  // Clean up
  for (int j = 0; j < m; ++j) bn_free(e[j]);
  delete[] e;
  return req;
}

std::vector<std::string> Client::decryptResults(
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

}  // namespace nomos
