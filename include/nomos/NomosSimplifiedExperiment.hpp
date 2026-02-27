#pragma once

#include <iostream>
#include <memory>

#include "core/Experiment.hpp"
#include "nomos/GatekeeperCorrect.hpp"
#include "nomos/ClientCorrect.hpp"
#include "nomos/ServerCorrect.hpp"

namespace nomos {

/**
 * @brief Simplified Nomos experiment with correct cryptographic operations
 *
 * This experiment demonstrates a simplified but cryptographically correct
 * implementation of the Nomos protocol:
 * - Uses elliptic curve operations (not SHA256 hashing)
 * - Uses key arrays Kt[1..d], Kx[1..d]
 * - Correctly implements Update (Algorithm 2)
 * - Correctly implements Search (Algorithm 4)
 * - Simplifies OPRF blinding (Gatekeeper computes directly)
 */
class NomosSimplifiedExperiment : public core::Experiment {
public:
    NomosSimplifiedExperiment()
        : m_gatekeeper(), m_client(), m_server() {}

    ~NomosSimplifiedExperiment() override = default;

    int setup() override {
        std::cout << "[Nomos-Simplified] Setting up..." << std::endl;

        // Initialize RELIC
        if (core_init() != RLC_OK) {
            std::cerr << "Failed to initialize RELIC" << std::endl;
            return -1;
        }

        // Set elliptic curve
        if (ep_param_set_any_pairf() != RLC_OK) {
            std::cerr << "Failed to set elliptic curve" << std::endl;
            return -1;
        }

        // Setup gatekeeper with d=10 key arrays
        if (m_gatekeeper.setup(10) != 0) {
            std::cerr << "Failed to setup gatekeeper" << std::endl;
            return -1;
        }

        // Setup client
        if (m_client.setup() != 0) {
            std::cerr << "Failed to setup client" << std::endl;
            return -1;
        }

        std::cout << "[Nomos-Simplified] Setup complete" << std::endl;
        return 0;
    }

    void run() override;

    void teardown() override {
        std::cout << "[Nomos-Simplified] Teardown complete" << std::endl;
        core_clean();
    }

    std::string getName() const override {
        return "Nomos-Simplified";
    }

private:
    GatekeeperCorrect m_gatekeeper;
    ClientCorrect m_client;
    ServerCorrect m_server;
};

}  // namespace nomos
