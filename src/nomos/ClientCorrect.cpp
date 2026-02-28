#include "nomos/ClientCorrect.hpp"

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

ClientCorrect::ClientCorrect() : m_n(0), m_m(0) {}

ClientCorrect::~ClientCorrect() {
  // No cleanup needed for simplified version
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

}  // namespace nomos
