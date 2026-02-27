#pragma once

#include <string>
#include <vector>

extern "C" {
#include <relic/relic.h>
}

namespace verifiable {

/**
 * @brief Embedded Commitment mechanism for address binding
 *
 * Implements the commitment scheme from Chapter 3 (commitment.tex)
 * Provides address correctness guarantee
 */
class AddressCommitment {
public:
    AddressCommitment();
    ~AddressCommitment();

    /**
     * @brief Compute commitment for a set of xtags
     * @param xtags The complete set of ℓ cross tags
     * @return Commitment value Cm_{w,id}
     */
    std::string commit(const std::vector<std::string>& xtags);

    /**
     * @brief Verify commitment opening
     * @param commitment The commitment value Cm_{w,id}
     * @param xtags The opened ℓ xtags
     * @return true if commitment matches
     */
    bool verify(const std::string& commitment, const std::vector<std::string>& xtags);

    /**
     * @brief Check subset membership
     * @param sampled_xtags The k sampled xtags from token
     * @param beta_indices The k indices β_1, ..., β_k
     * @param full_xtags The ℓ xtags from opening
     * @return true if sampled_xtags[i] == full_xtags[beta_indices[i]]
     */
    static bool checkSubsetMembership(const std::vector<std::string>& sampled_xtags,
                                     const std::vector<int>& beta_indices,
                                     const std::vector<std::string>& full_xtags);

private:
    std::string hashCommitment(const std::vector<std::string>& xtags);
};

}  // namespace verifiable
