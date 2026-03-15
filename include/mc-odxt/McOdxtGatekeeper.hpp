#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "mc-odxt/McOdxtTypes.hpp"

namespace mcodxt {

class McOdxtGatekeeper {
 public:
  McOdxtGatekeeper();
  ~McOdxtGatekeeper();

  int setup(int d = 10);

  UpdateMetadata update(OpType op, const std::string& id,
                        const std::string& keyword);

  int getUpdateCount(const std::string& keyword) const;

  const std::unordered_map<std::string, int>& getUpdateCounts() const {
    return m_updateCnt;
  }

  void setUpdateCountForBenchmark(const std::string& keyword, int count);

  const std::vector<uint8_t>& getKm() const { return m_Km; }

  /**
   * @brief Gatekeeper side of Nomos GenToken for MC-ODXT without RBF
   *
   * @param req Client-generated request containing the reordered query and
   *            pre-hashed group elements
   * @return SearchToken with gatekeeper-applied search tokens
   */
  SearchToken genToken(const TokenRequest& req);

 private:
  bn_t m_Ks;
  bn_t* m_Kt;
  bn_t* m_Kx;
  bn_t m_Ky;
  std::vector<uint8_t> m_Km;
  std::unordered_map<std::string, int> m_updateCnt;
  int m_d;

  int indexFunction(const std::string& keyword) const;
  std::string computeKz(const std::string& keyword);
  void computeF_p(bn_t result, const bn_t key, const std::string& input);
  void computeF_p(bn_t result, const std::string& key,
                  const std::string& input);
};

}  // namespace mcodxt
