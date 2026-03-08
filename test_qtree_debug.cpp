#include <iostream>
#include <vector>
#include "verifiable/QTree.hpp"

int main() {
    // Initialize RELIC
    if (core_init() != RLC_OK) {
        std::cerr << "RELIC init failed" << std::endl;
        return 1;
    }

    verifiable::QTree qtree(1024);
    std::vector<bool> initial_bits(1024, false);
    qtree.initialize(initial_bits);

    std::string addr1 = "addr_001";
    
    // Update bit
    qtree.updateBit(addr1, true);
    std::string root_after_update = qtree.getRootHash();
    
    // Generate proof
    auto proof = qtree.generateProof(addr1);
    std::cout << "Proof length: " << proof.size() << std::endl;
    std::cout << "Root hash: " << root_after_update.substr(0, 16) << "..." << std::endl;
    
    // Verify proof
    bool verified = qtree.verifyPath(addr1, true, proof, root_after_update);
    std::cout << "Verification: " << (verified ? "PASS" : "FAIL") << std::endl;
    
    // Compute index
    size_t index = std::hash<std::string>{}(addr1) % 1024;
    std::cout << "Address '" << addr1 << "' maps to index: " << index << std::endl;

    core_clean();
    return 0;
}
