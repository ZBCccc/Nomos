#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "types.hpp"

extern "C" {
#include <relic/relic.h>
}

namespace nomos {

/**
 * @brief Nomos Gatekeeper implementation according to Algorithms 2-3
 */
class Gatekeeper {
 public:
  Gatekeeper();
  ~Gatekeeper();

  /**
   * @brief Setup - Algorithm 1
   * Initialize keys: Ks, Kt[1..d], Kx[1..d], Ky, Km
   */
  int setup(int d = 10);  // d = number of key array elements

  /**
   * @brief Update - Algorithm 2
   * @param op Operation (ADD/DEL)
   * @param id Document identifier
   * @param keyword Keyword
   * @return UpdateMetadata to send to server
   */
  UpdateMetadata update(OP op, const std::string& id,
                        const std::string& keyword);

  /**
   * @brief Get update count for a keyword
   */
  int getUpdateCount(const std::string& keyword) const;

  /**
   * @brief Get all update counts (for client)
   */
  const std::unordered_map<std::string, int>& getUpdateCounts() const {
    return m_updateCnt;
  }

  /**
   * @brief Benchmark helper: override keyword update count without replaying
   * all updates
   */
  void setUpdateCountForBenchmark(const std::string& keyword, int count);

  /**
   * @brief Get encryption key K_M for Server setup
   */
  const std::vector<uint8_t>& getKm() const { return m_Km; }

  /**
   * @brief Gatekeeper side of Nomos GenToken for the experimental path
   *
   * Applies the master keys to the client-generated hash points while
   * preserving the RBF-based bxtrap derivation from the paper.
   *
   * @param req Client-generated token request
   * @return SearchToken with gatekeeper-applied search tokens
   */
  SearchToken genToken(const TokenRequest& req);

 private:
  // Keys
  bn_t m_Ks;   // Base secret exponent
  bn_t* m_Kt;  // TSet key array (manually managed)
  bn_t* m_Kx;  // XSet key array (manually managed)
  bn_t m_Ky;   // F_p key, stored as a scalar and serialized on use
  std::vector<uint8_t> m_Km;  // AE key

  // State
  std::unordered_map<std::string, int> m_updateCnt;  // UpdateCnt[w]
  int m_d;                                           // Key array size

  // Helper functions
  int indexFunction(const std::string& keyword) const;  // I(w)
  std::string computeKz(
      const std::string& keyword);  // Kz = F(serialize(H(w)^Ks), "1")
  void computeF_p(bn_t result, const bn_t key,
                  const std::string& input);  // F_p(key, input)
  void computeF_p(bn_t result, const std::string& key,
                  const std::string& input);  // F_p(key, input)
};

}  // namespace nomos
