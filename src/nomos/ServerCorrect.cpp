#include "nomos/ServerCorrect.hpp"
#include "core/Primitive.hpp"

#include <sstream>

namespace nomos {

// Helper function to deserialize string to elliptic curve point
static void deserializePoint(ep_t point, const std::string& str) {
    ep_read_bin(point, reinterpret_cast<const uint8_t*>(str.data()), str.length());
}

ServerCorrect::ServerCorrect() {}

ServerCorrect::~ServerCorrect() {}

std::string ServerCorrect::serializePoint(const ep_t point) const {
    uint8_t bytes[256];
    int len = ep_size_bin(point, 1);
    ep_write_bin(bytes, len, point, 1);
    return std::string(reinterpret_cast<char*>(bytes), len);
}

void ServerCorrect::update(const UpdateMetadata& meta) {
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

std::vector<SearchResultEntry> ServerCorrect::search(
    const ClientCorrect::SearchRequest& req) {

    std::vector<SearchResultEntry> results;

    // Decrypt env to get (rho, gamma)
    // For simplified version, we skip this as we don't use gamma for unblinding
    // We directly use stokenList

    int m = req.stokenList.size();
    int n = req.xtokenList.empty() ? 0 : req.xtokenList[0].size() + 1;

    // For each stoken_j
    for (int j = 0; j < m; ++j) {
        const std::string& stag_key = req.stokenList[j];

        // Lookup (val, alpha) = TSet[stag]
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

                // For each xtoken[i][j][t], compute xtag = xtoken^{alpha/rho_i}
                // For simplified version, we use alpha directly
                for (const auto& xtoken_str : xtokens) {
                    ep_t xtoken;
                    ep_new(xtoken);
                    deserializePoint(xtoken, xtoken_str);

                    ep_t xtag;
                    ep_new(xtag);
                    ep_mul(xtag, xtoken, entry.alpha);

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

    return results;
}

}  // namespace nomos
