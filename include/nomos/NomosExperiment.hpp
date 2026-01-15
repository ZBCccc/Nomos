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
        // Basic simulation of the protocol
        std::string keyword = "test_keyword";
        std::string id = "doc1";

        // 1. Gatekeeper updates
        std::cout << "[Nomos] Gatekeeper processing update..." << std::endl;
        Metadata meta = m_gatekeeper->Update(OP_ADD, id, keyword);

        // 2. Server updates
        std::cout << "[Nomos] Server applying update..." << std::endl;
        m_server->Update(meta);

        std::cout << "[Nomos] Experiment run complete." << std::endl;
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
