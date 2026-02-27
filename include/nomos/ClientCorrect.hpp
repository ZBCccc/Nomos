#pragma once

#include <memory>
#include <string>
#include <vector>

#include "types_correct.hpp"
#include "GatekeeperCorrect.hpp"

extern "C" {
#include <relic/relic.h>
}

namespace nomos {

/**
 * @brief Correct Nomos Client implementation according to Algorithm 3-4
 */
class ClientCorrect {
public:
    ClientCorrect();
    ~ClientCorrect();

    int setup();

    /**
     * @brief GenToken - Algorithm 3 (Client side) - FULL OPRF VERSION
     * Phase 1: Blind keywords and send to gatekeeper
     * Phase 2: Unblind received tokens
     * NOTE: Not used in simplified implementation
     */
    /*
    struct BlindedRequest {
        std::vector<ep_t> a;  // a_j = H(wj)^rj
        std::vector<ep_t> b;  // b_j = H(w1||j||0)^sj
        std::vector<ep_t> c;  // c_j = H(w1||j||1)^sj
        std::vector<int> av;  // Access vector
    };

    BlindedRequest genTokenPhase1(
        const std::vector<std::string>& query_keywords,
        const std::unordered_map<std::string, int>& updateCnt);

    SearchToken genTokenPhase2(
        const GatekeeperCorrect::BlindedTokens& blinded_tokens);
    */

    /**
     * @brief Generate search token (simplified version - no OPRF blinding)
     *
     * Gatekeeper directly computes all tokens without interactive blinding.
     * This is cryptographically correct but skips the privacy-preserving OPRF step.
     *
     * @param query_keywords Query keywords [w1, ..., wn]
     * @param gatekeeper Reference to gatekeeper
     * @return SearchToken with pre-computed tokens
     */
    SearchToken genTokenSimplified(
        const std::vector<std::string>& query_keywords,
        GatekeeperCorrect& gatekeeper);

    /**
     * @brief Search - Algorithm 4 (Client side)
     * Compute xtokens from search token
     */
    struct SearchRequest {
        std::vector<std::string> stokenList;                    // Serialized stokens
        std::vector<std::vector<std::vector<std::string>>> xtokenList;  // [j][i][t] serialized
        std::vector<uint8_t> env;

        SearchRequest() {}
        ~SearchRequest() {}
    };

    SearchRequest prepareSearch(const SearchToken& token,
                                const std::vector<std::string>& query_keywords,
                                const std::unordered_map<std::string, int>& updateCnt);

    /**
     * @brief Decrypt search results
     */
    std::vector<std::string> decryptResults(
        const std::vector<SearchResultEntry>& results,
        const SearchToken& token);

private:
    // Blinding factors (stored between phase 1 and 2) - NOT USED IN SIMPLIFIED VERSION
    // std::vector<bn_t> m_r;  // r_1, ..., r_n
    // std::vector<bn_t> m_s;  // s_1, ..., s_m
    int m_n;  // Number of query keywords
    int m_m;  // UpdateCnt[w1]
};

}  // namespace nomos
