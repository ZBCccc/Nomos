#include "mc-odxt/McOdxtExperiment.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>

namespace mcodxt {

McOdxtExperiment::McOdxtExperiment() {}

McOdxtExperiment::~McOdxtExperiment() {}

int McOdxtExperiment::setup() {
    std::cout << "[MC-ODXT] Setting up..." << std::endl;

    if (m_gatekeeper.setup(10) != 0) {
        std::cerr << "Failed to setup gatekeeper" << std::endl;
        return -1;
    }

    if (m_client.setup() != 0) {
        std::cerr << "Failed to setup client" << std::endl;
        return -1;
    }

    m_server.setup(m_gatekeeper.getKm());
    return 0;
}

void McOdxtExperiment::run() {
    std::cout << "\n[MC-ODXT] Running map-backed experiment..." << std::endl;

    m_server.update(m_gatekeeper.update(OpType::ADD, "doc1", "crypto"));
    m_server.update(m_gatekeeper.update(OpType::ADD, "doc1", "security"));
    m_server.update(m_gatekeeper.update(OpType::ADD, "doc2", "security"));
    m_server.update(m_gatekeeper.update(OpType::ADD, "doc2", "privacy"));
    m_server.update(m_gatekeeper.update(OpType::ADD, "doc3", "crypto"));
    m_server.update(m_gatekeeper.update(OpType::ADD, "doc3", "blockchain"));

    {
        const std::vector<std::string> query = {"crypto", "security"};
        SearchToken token = m_client.genTokenSimplified(query, m_gatekeeper);
        McOdxtClient::SearchRequest req =
            m_client.prepareSearch(token, query, m_gatekeeper.getUpdateCounts());
        std::vector<SearchResultEntry> results = m_server.search(req);
        std::vector<std::string> ids = m_client.decryptResults(results, token);
        std::sort(ids.begin(), ids.end());
        assert(ids.size() == 1);
        assert(ids[0] == "doc1");
    }

    {
        const std::vector<std::string> query = {"crypto"};
        SearchToken token = m_client.genTokenSimplified(query, m_gatekeeper);
        McOdxtClient::SearchRequest req =
            m_client.prepareSearch(token, query, m_gatekeeper.getUpdateCounts());
        std::vector<SearchResultEntry> results = m_server.search(req);
        std::vector<std::string> ids = m_client.decryptResults(results, token);
        std::sort(ids.begin(), ids.end());
        assert(ids.size() == 2);
        assert(ids[0] == "doc1");
        assert(ids[1] == "doc3");
    }

    std::cout << "[MC-ODXT] All checks passed." << std::endl;
}

void McOdxtExperiment::teardown() {
    std::cout << "[MC-ODXT] Teardown complete." << std::endl;
}

std::string McOdxtExperiment::getName() const {
    return "mc-odxt";
}

}  // namespace mcodxt
