#pragma once

#include <map>
#include <string>
#include <vector>

#include "types_correct.hpp"
#include "ClientCorrect.hpp"

extern "C" {
#include <relic/relic.h>
}

namespace nomos {

/**
 * @brief Correct Nomos Server implementation according to Algorithm 2 & 4
 */
class ServerCorrect {
public:
    ServerCorrect();
    ~ServerCorrect();

    /**
     * @brief Update - Algorithm 2 (Server side)
     * Store (addr, val, alpha) in TSet and xtags in XSet
     */
    void update(const UpdateMetadata& meta);

    /**
     * @brief Search - Algorithm 4 (Server side)
     * @param req Search request from client
     * @return Search results
     */
    std::vector<SearchResultEntry> search(const ClientCorrect::SearchRequest& req);

    /**
     * @brief Get TSet size (for testing)
     */
    size_t getTSetSize() const { return m_TSet.size(); }

    /**
     * @brief Get XSet size (for testing)
     */
    size_t getXSetSize() const { return m_XSet.size(); }

private:
    // TSet: ep_t (addr) -> TSetEntry (val, alpha)
    std::map<std::string, TSetEntry> m_TSet;

    // XSet: ep_t (xtag) -> bool
    std::map<std::string, bool> m_XSet;

    // Helper: serialize ep_t to string for map key
    std::string serializePoint(const ep_t point) const;
};

}  // namespace nomos
