#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "types_correct.hpp"

extern "C" {
#include <relic/relic.h>
}

namespace nomos {

/**
 * @brief Correct Nomos Gatekeeper implementation according to Algorithm 2-3
 */
class GatekeeperCorrect {
public:
    GatekeeperCorrect();
    ~GatekeeperCorrect();

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
    UpdateMetadata update(OP op, const std::string& id, const std::string& keyword);

    /**
     * @brief GenToken - Algorithm 3 (Gatekeeper side) - FULL OPRF VERSION
     * NOTE: Not used in simplified implementation
     * @param query_keywords Query keywords [w1, ..., wn]
     * @param blinded_a Client's blinded a_j values
     * @param blinded_b Client's blinded b_j values
     * @param blinded_c Client's blinded c_j values
     * @param av Access vector (I(w1), ..., I(wn))
     * @return Blinded tokens to send back to client
     */
    /*
    struct BlindedTokens {
        ep_t strap_prime;
        std::vector<ep_t> bstag_prime;
        std::vector<ep_t> delta_prime;
        std::vector<std::vector<ep_t>> bxtrap_prime;
        std::vector<uint8_t> env;
    };

    BlindedTokens genTokenGatekeeper(
        const std::vector<std::string>& query_keywords,
        const std::vector<ep_t>& blinded_a,
        const std::vector<ep_t>& blinded_b,
        const std::vector<ep_t>& blinded_c,
        const std::vector<int>& av);
    */

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
     * @brief Generate tokens directly (simplified version - no OPRF blinding)
     *
     * Computes all tokens directly without interactive blinding protocol.
     * This is cryptographically correct but skips the privacy-preserving OPRF step.
     *
     * @param query_keywords Query keywords [w1, ..., wn]
     * @return SearchToken with pre-computed tokens
     */
    SearchToken genTokenSimplified(const std::vector<std::string>& query_keywords);

private:
    // Keys
    bn_t m_Ks;                          // OPRF key
    bn_t* m_Kt;                         // TSet key array (manually managed)
    bn_t* m_Kx;                         // XSet key array (manually managed)
    bn_t m_Ky;                          // PRF key
    std::vector<uint8_t> m_Km;          // AE key

    // State
    std::unordered_map<std::string, int> m_updateCnt;  // UpdateCnt[w]
    int m_d;                            // Key array size

    // Helper functions
    int indexFunction(const std::string& keyword) const;  // I(w)
    void computeKz(bn_t kz, const std::string& keyword);  // Kz = F((H(w))^Ks, 1)
    void computeFp(bn_t result, bn_t key, const std::string& input);  // Fp(key, input)
};

}  // namespace nomos
