#pragma once

#include <iostream>
#include <memory>

#include "core/Experiment.hpp"
#include "verifiable/QTree.hpp"
#include "verifiable/AddressCommitment.hpp"
// Note: Verifiable experiment currently uses standalone QTree/Commitment testing
// Integration with Nomos Correct implementation is pending

namespace verifiable {

class VerifiableExperiment : public core::Experiment {
public:
    VerifiableExperiment()
        : m_qtree(new QTree(1024)),  // 1024 XSet capacity
          m_commitment(new AddressCommitment()) {}

    int setup() override {
        std::cout << "[Verifiable] Setting up..." << std::endl;

        // Initialize QTree with empty bit array
        std::vector<bool> initial_bits(1024, false);
        m_qtree->initialize(initial_bits);

        std::cout << "[Verifiable] QTree initialized with root: "
                  << m_qtree->getRootHash().substr(0, 16) << "..." << std::endl;
        return 0;
    }

    void run() override {
        std::cout << "[Verifiable] Running experiment..." << std::endl;

        // Test QTree functionality
        std::cout << "\n[Verifiable] Testing QTree..." << std::endl;
        std::string addr1 = "addr_001";
        std::string addr2 = "addr_002";

        m_qtree->updateBit(addr1, true);
        m_qtree->updateBit(addr2, false);

        std::cout << "  Updated bits at " << addr1 << " (1) and " << addr2 << " (0)" << std::endl;
        std::cout << "  New root: " << m_qtree->getRootHash().substr(0, 16) << "..." << std::endl;
        std::cout << "  Version: " << m_qtree->getVersion() << std::endl;

        // Generate and verify proof
        auto proof1 = m_qtree->generateProof(addr1);
        std::cout << "  Generated proof for " << addr1 << " (length: " << proof1.size() << ")" << std::endl;

        bool verified = QTree::verifyPath(addr1, true, proof1, m_qtree->getRootHash());
        std::cout << "  Verification result: " << (verified ? "PASS" : "FAIL") << std::endl;

        // Test Address Commitment
        std::cout << "\n[Verifiable] Testing Address Commitment..." << std::endl;
        std::vector<std::string> xtags = {"xtag1", "xtag2", "xtag3"};
        std::string cm = m_commitment->commit(xtags);
        std::cout << "  Commitment: " << cm.substr(0, 16) << "..." << std::endl;

        bool cm_verified = m_commitment->verify(cm, xtags);
        std::cout << "  Commitment verification: " << (cm_verified ? "PASS" : "FAIL") << std::endl;

        // Test subset membership
        std::vector<std::string> sampled = {"xtag1", "xtag3"};
        std::vector<int> beta = {1, 3};
        bool subset_ok = AddressCommitment::checkSubsetMembership(sampled, beta, xtags);
        std::cout << "  Subset membership check: " << (subset_ok ? "PASS" : "FAIL") << std::endl;

        std::cout << "\n[Verifiable] Experiment run complete." << std::endl;
    }

    void teardown() override {
        std::cout << "[Verifiable] Tearing down..." << std::endl;
    }

    std::string getName() const override {
        return "verifiable";
    }

private:
    std::unique_ptr<QTree> m_qtree;
    std::unique_ptr<AddressCommitment> m_commitment;
};

}  // namespace verifiable
