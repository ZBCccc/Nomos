#pragma once

#include <memory>
#include <string>
#include <vector>

#include "types.hpp"
#include "Gatekeeper.hpp"

extern "C" {
#include <relic/relic.h>
}

namespace nomos {

/**
 * @brief Nomos Client implementation according to Algorithm 3-4
 */
class Client {
public:
    Client();
    ~Client();

    int setup();

    /**
     * @brief GenToken Phase 1 - Algorithm 3 (Client side, lines 1-6)
     *
     * Generates blinded request to send to Gatekeeper.
     * Client blinds keywords with random factors r_j, s_j to hide query content.
     *
     * @param query_keywords Query keywords [w1, ..., wn]
     * @param updateCnt UpdateCnt map from Gatekeeper
     * @return BlindedRequest to send to Gatekeeper
     */
    BlindedRequest genTokenPhase1(
        const std::vector<std::string>& query_keywords,
        const std::unordered_map<std::string, int>& updateCnt);

    /**
     * @brief GenToken Phase 2 - Algorithm 3 (Client side, lines 15-19)
     *
     * Unblinds response from Gatekeeper to obtain final search token.
     * Client removes blinding factors r_j^{-1}, s_j^{-1} to get usable tokens.
     *
     * @param response Blinded response from Gatekeeper
     * @return SearchToken ready for search
     */
    SearchToken genTokenPhase2(const BlindedResponse& response);

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
        Gatekeeper& gatekeeper);

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
    // Blinding factors (stored between phase 1 and 2)
    // Note: bn_t is array type (bn_st[1]), cannot use std::vector
    bn_t* m_r;  // r_1, ..., r_n (random blinding factors for keywords)
    bn_t* m_s;  // s_1, ..., s_m (random blinding factors for stags)
    int m_n;  // Number of query keywords
    int m_m;  // UpdateCnt[w1]

    // Helper: free blinding factors
    void freeBlindingFactors();
};

}  // namespace nomos
