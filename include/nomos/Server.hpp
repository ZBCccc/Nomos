#pragma once

#include <map>
#include <string>
#include <vector>

#include "types.hpp"
#include "Client.hpp"

extern "C" {
#include <relic/relic.h>
}

namespace nomos {

/**
 * @brief Nomos Server implementation according to Algorithm 2 & 4
 */
class Server {
public:
    Server();
    ~Server();

    /**
     * @brief Setup server with decryption key for env
     * @param Km Decryption key from Gatekeeper
     */
    void setup(const std::vector<uint8_t>& Km);

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
    std::vector<SearchResultEntry> search(const Client::SearchRequest& req);

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

    // Decryption key for env (from Gatekeeper)
    std::vector<uint8_t> m_Km;

    // Helper: serialize ep_t to string for map key
    std::string serializePoint(const ep_t point) const;
};

}  // namespace nomos
