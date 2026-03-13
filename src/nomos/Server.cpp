#include "nomos/Server.hpp"

#include <sstream>

#include "core/Primitive.hpp"

namespace nomos {

// Helper function to deserialize string to elliptic curve point
static void deserializePoint(ep_t point, const std::string& str) {
  ep_read_bin(point, reinterpret_cast<const uint8_t*>(str.data()),
              str.length());
}

Server::Server() {}

Server::~Server() {
  // Explicitly clear TSet to ensure TSetEntry destructors are called
  // This frees all bn_t alpha values stored in TSet entries
  m_TSet.clear();
  m_XSet.clear();
}

void Server::setup(const std::vector<uint8_t>& Km) {
  m_Km = Km;
}

std::string Server::serializePoint(const ep_t point) const {
  uint8_t bytes[256];
  int len = ep_size_bin(point, 1);
  ep_write_bin(bytes, len, point, 1);
  return std::string(reinterpret_cast<char*>(bytes), len);
}

void Server::update(const UpdateMetadata& meta) {
  // Step 1: Serialize addr to string key
  std::string addr_key = serializePoint(meta.addr);

  // Step 2: Store TSet[addr] = (val, alpha)
  TSetEntry entry;
  entry.val = meta.val;
  bn_new(entry.alpha);
  bn_copy(entry.alpha, meta.alpha);

  m_TSet[addr_key] = entry;

  // Step 3: Store xtags in XSet
  for (const auto& xtag_str : meta.xtags) {
    m_XSet[xtag_str] = true;
  }
}

std::vector<SearchResultEntry> Server::search(const Client::SearchRequest& req) {
  std::vector<SearchResultEntry> results;

  int m = req.stokenList.size();
  int n = req.xtokenList.empty() ? 0 : req.xtokenList[0].size() + 1;

  // Decrypt env to get (rho, gamma) - Paper: Algorithm 4, line 1
  bn_t* rho = nullptr;
  bn_t* gamma = nullptr;
  bool has_env = false;

  if (!req.env.empty() && !m_Km.empty()) {
    // XOR decryption with K_M
    std::vector<uint8_t> plaintext_env(req.env.size());
    for (size_t i = 0; i < req.env.size(); ++i) {
      plaintext_env[i] = req.env[i] ^ m_Km[i % m_Km.size()];
    }

    // Parse rho and gamma from plaintext
    // Note: This is simplified - proper implementation needs length encoding
    // For now, assume fixed-size bn_t serialization (32 bytes each)
    const int BN_SIZE = 32;
    int expected_size = (n + m) * BN_SIZE;

    if (plaintext_env.size() >= static_cast<size_t>(expected_size)) {
      bn_t ord;
      bn_new(ord);
      ep_curve_get_ord(ord);

      // Allocate and deserialize rho values
      rho = new bn_t[n];
      for (int i = 0; i < n; ++i) {
        bn_new(rho[i]);
        bn_read_bin(rho[i], &plaintext_env[i * BN_SIZE], BN_SIZE);
      }

      // Allocate and deserialize gamma values
      gamma = new bn_t[m];
      for (int j = 0; j < m; ++j) {
        bn_new(gamma[j]);
        bn_read_bin(gamma[j], &plaintext_env[(n + j) * BN_SIZE], BN_SIZE);
      }

      bn_free(ord);
      has_env = true;
    }
  }

  // For each stoken_j (Paper: Algorithm 4, lines 3-14)
  for (int j = 0; j < m; ++j) {
    std::string stag_key;

    // If we have gamma, unblind: stag_j = (stokenList[j])^{1/gamma_j}
    // Paper: Algorithm 4, line 5
    if (has_env && gamma != nullptr) {
      bn_t ord, gamma_inv;
      bn_new(ord);
      bn_new(gamma_inv);
      ep_curve_get_ord(ord);
      bn_mod_inv(gamma_inv, gamma[j], ord);

      ep_t stoken, stag;
      ep_new(stoken);
      ep_new(stag);
      deserializePoint(stoken, req.stokenList[j]);
      ep_mul(stag, stoken, gamma_inv);
      stag_key = serializePoint(stag);

      ep_free(stoken);
      ep_free(stag);
      bn_free(ord);
      bn_free(gamma_inv);
    } else {
      // Simplified version: use stokenList directly
      stag_key = req.stokenList[j];
    }

    // Lookup (val, alpha) = TSet[stag] - Paper: Algorithm 4, line 6
    auto it = m_TSet.find(stag_key);
    if (it == m_TSet.end()) {
      continue;  // No match
    }

    const TSetEntry& entry = it->second;

    // Check cross-filtering: for each xtoken, verify xtag in XSet
    bool all_match = true;
    int match_count = 0;

    if (n > 1) {  // Has cross-filtering keywords
      for (int i = 0; i < n - 1; ++i) {
        if (j >= static_cast<int>(req.xtokenList.size())) {
          all_match = false;
          break;
        }
        if (i >= static_cast<int>(req.xtokenList[j].size())) {
          all_match = false;
          break;
        }

        const auto& xtokens = req.xtokenList[j][i];
        bool keyword_match = false;

        // For each xtoken[i][j][t], compute xtag = xtoken^{alpha/(rho_i * beta_t)}
        // Paper: Algorithm 4, lines 8-11
        for (const auto& xtoken_str : xtokens) {
          ep_t xtoken;
          ep_new(xtoken);
          deserializePoint(xtoken, xtoken_str);

          ep_t xtag;
          ep_new(xtag);

          // If we have rho, compute xtag = xtoken^{alpha/rho_{i+1}}
          // Otherwise use alpha directly (simplified version)
          if (has_env && rho != nullptr && (i + 1) < n) {
            bn_t ord, rho_inv, exp;
            bn_new(ord);
            bn_new(rho_inv);
            bn_new(exp);
            ep_curve_get_ord(ord);

            // exp = alpha / rho_{i+1}
            bn_mod_inv(rho_inv, rho[i + 1], ord);
            bn_mul(exp, entry.alpha, rho_inv);
            bn_mod(exp, exp, ord);

            ep_mul(xtag, xtoken, exp);

            bn_free(ord);
            bn_free(rho_inv);
            bn_free(exp);
          } else {
            // Simplified: xtag = xtoken^alpha
            ep_mul(xtag, xtoken, entry.alpha);
          }

          std::string xtag_key = serializePoint(xtag);
          if (m_XSet.find(xtag_key) != m_XSet.end()) {
            keyword_match = true;
            match_count++;
          }

          ep_free(xtag);
          ep_free(xtoken);

          if (keyword_match) break;  // Found match for this keyword
        }

        if (!keyword_match) {
          all_match = false;
          break;
        }
      }
    }

    // If all keywords match, add to results
    if (all_match) {
      SearchResultEntry result;
      result.j = j + 1;  // 1-indexed
      result.sval = entry.val;
      result.cnt = match_count;
      results.push_back(result);
    }
  }

  // Clean up rho and gamma
  if (rho != nullptr) {
    for (int i = 0; i < n; ++i) {
      bn_free(rho[i]);
    }
    delete[] rho;
  }
  if (gamma != nullptr) {
    for (int j = 0; j < m; ++j) {
      bn_free(gamma[j]);
    }
    delete[] gamma;
  }

  return results;
}

}  // namespace nomos
