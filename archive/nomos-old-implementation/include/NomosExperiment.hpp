#pragma once

#include <iostream>
#include <memory>

#include "core/Experiment.hpp"
#include "nomos/Client.hpp"
#include "nomos/Gatekeeper.hpp"
#include "nomos/Server.hpp"

namespace nomos {

class NomosExperiment : public core::Experiment {
public:
    NomosExperiment()
        : m_client(new Client()),
          m_gatekeeper(new GateKeeper()),
          m_server(new Server()) {}

    int setup() override {
        std::cout << "[Nomos] Setting up..." << std::endl;
        if (m_client->Setup() != 0) return -1;
        if (m_gatekeeper->Setup() != 0) return -1;
        return 0;
    }

    void run() override {
        std::cout << "[Nomos] Running experiment..." << std::endl;

        // Test data
        std::vector<std::string> keywords = {"crypto", "security", "privacy"};
        std::vector<std::string> doc_ids = {"doc1", "doc2", "doc3"};

        // Phase 1: Update - add documents
        std::cout << "[Nomos] Phase 1: Adding documents..." << std::endl;
        for (const auto& doc_id : doc_ids) {
            for (const auto& kw : keywords) {
                if ((doc_id == "doc1" && (kw == "crypto" || kw == "security")) ||
                    (doc_id == "doc2" && (kw == "security" || kw == "privacy")) ||
                    (doc_id == "doc3" && kw == "crypto")) {
                    std::cout << "  Adding: " << doc_id << " with keyword " << kw << std::endl;
                    Metadata meta = m_gatekeeper->Update(OP_ADD, doc_id, kw);
                    m_server->Update(meta);
                }
            }
        }

        // Phase 2: Search - query for documents
        std::cout << "\n[Nomos] Phase 2: Searching..." << std::endl;

        // Query 1: [crypto, security] - should match doc1
        std::cout << "\n  Query 1: [crypto, security]" << std::endl;
        std::vector<std::string> query1 = {"crypto", "security"};
        TrapdoorMetadata trapdoor1 = m_gatekeeper->GenToken(query1);
        std::vector<sEOp> results1 = m_server->Search(trapdoor1);
        std::cout << "  Found " << results1.size() << " matching documents" << std::endl;
        for (const auto& result : results1) {
            std::cout << "    - Document: " << result.id << std::endl;
        }

        // Query 2: [security, privacy] - should match doc2
        std::cout << "\n  Query 2: [security, privacy]" << std::endl;
        std::vector<std::string> query2 = {"security", "privacy"};
        TrapdoorMetadata trapdoor2 = m_gatekeeper->GenToken(query2);
        std::vector<sEOp> results2 = m_server->Search(trapdoor2);
        std::cout << "  Found " << results2.size() << " matching documents" << std::endl;
        for (const auto& result : results2) {
            std::cout << "    - Document: " << result.id << std::endl;
        }

        // Query 3: [crypto] - should match doc1 and doc3
        std::cout << "\n  Query 3: [crypto]" << std::endl;
        std::vector<std::string> query3 = {"crypto"};
        TrapdoorMetadata trapdoor3 = m_gatekeeper->GenToken(query3);
        std::vector<sEOp> results3 = m_server->Search(trapdoor3);
        std::cout << "  Found " << results3.size() << " matching documents" << std::endl;
        for (const auto& result : results3) {
            std::cout << "    - Document: " << result.id << std::endl;
        }

        std::cout << "\n[Nomos] Experiment run complete." << std::endl;
    }

    void teardown() override {
        std::cout << "[Nomos] Tearing down..." << std::endl;
    }

    std::string getName() const override {
        return "nomos";
    }

private:
    std::unique_ptr<Client> m_client;
    std::unique_ptr<GateKeeper> m_gatekeeper;
    std::unique_ptr<Server> m_server;
};

}  // namespace nomos
