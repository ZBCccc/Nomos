#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "vq-nomos/QTree.hpp"
#include "vq-nomos/types.hpp"

extern "C" {
#include <openssl/evp.h>
}

namespace vqnomos {

class Gatekeeper {
 public:
  Gatekeeper();
  ~Gatekeeper();

  int setup(int d = 10, size_t qtree_capacity = 1024);

  UpdateMetadata update(OP op, const std::string& id,
                        const std::string& keyword);

  int getUpdateCount(const std::string& keyword) const;

  const std::unordered_map<std::string, int>& getUpdateCounts() const {
    return m_updateCnt;
  }

  const std::vector<uint8_t>& getKm() const { return m_Km; }

  SearchToken genToken(const TokenRequest& request);

  Anchor getCurrentAnchor() const;
  std::string getPublicKeyPem() const;

 private:
  int indexFunction(const std::string& keyword) const;
  std::string computeKz(const std::string& keyword);
  std::string signMerkleRoot(const std::string& keyword,
                             const std::string& root_hash) const;
  Anchor buildAnchor() const;

  bn_t m_Ks;
  bn_t* m_Kt;
  bn_t* m_Kx;
  bn_t m_Ky;
  std::vector<uint8_t> m_Km;

  std::unordered_map<std::string, int> m_updateCnt;
  int m_d;

  std::unique_ptr<QTree> m_qtree;
  EVP_PKEY* m_signing_key;
};

}  // namespace vqnomos
