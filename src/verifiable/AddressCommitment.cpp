#include "verifiable/AddressCommitment.hpp"

#include <sstream>

extern "C" {
#include <openssl/sha.h>
}

namespace verifiable {

AddressCommitment::AddressCommitment() {}

AddressCommitment::~AddressCommitment() {}

std::string AddressCommitment::hashCommitment(
    const std::vector<std::string>& xtags) {
  // Cm_{w,id} = H_c(xtag_1 || xtag_2 || ... || xtag_ℓ)
  std::stringstream ss;
  for (const auto& xtag : xtags) {
    ss << xtag;
  }
  std::string concatenated = ss.str();

  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(concatenated.c_str()),
         concatenated.length(), hash);

  return std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
}

std::string AddressCommitment::commit(const std::vector<std::string>& xtags) {
  if (xtags.empty()) {
    throw std::runtime_error("Cannot commit to empty xtag list");
  }
  return hashCommitment(xtags);
}

bool AddressCommitment::verify(const std::string& commitment,
                               const std::vector<std::string>& xtags) {
  std::string recomputed = hashCommitment(xtags);
  return recomputed == commitment;
}

bool AddressCommitment::checkSubsetMembership(
    const std::vector<std::string>& sampled_xtags,
    const std::vector<int>& beta_indices,
    const std::vector<std::string>& full_xtags) {
  if (sampled_xtags.size() != beta_indices.size()) {
    return false;
  }

  for (size_t i = 0; i < sampled_xtags.size(); ++i) {
    int beta = beta_indices[i];
    if (beta < 1 || beta > static_cast<int>(full_xtags.size())) {
      return false;
    }
    // β is 1-indexed, convert to 0-indexed
    if (sampled_xtags[i] != full_xtags[beta - 1]) {
      return false;
    }
  }

  return true;
}

}  // namespace verifiable
