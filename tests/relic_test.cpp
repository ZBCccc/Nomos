#include <iostream>
#include <gmp.h>

extern "C" {
#include <relic/relic.h>
}

int main() {
    std::cout << "=== RELIC Library Test ===" << std::endl;

    std::cout << "Initializing RELIC core..." << std::endl;
    if (core_init() != RLC_OK) {
        std::cerr << "Core init failed!" << std::endl;
        core_clean();
        return 1;
    }

    std::cout << "Initializing Pairing Cryptography (PC) parameters..." << std::endl;
    if (pc_param_set_any() != RLC_OK) {
        std::cerr << "PC param set failed!" << std::endl;
        core_clean();
        return 1;
    }

    std::cout << "RELIC core and params initialized successfully." << std::endl;

    // Test simple big number generation
    bn_t n;
    bn_new(n);
    bn_rand(n, RLC_POS, 256);
    std::cout << "Successfully generated a random 256-bit Big Number." << std::endl;
    bn_free(n);

    // Test simple G1 group operation
    ep_t p;
    ep_new(p);
    ep_rand(p);
    std::cout << "Successfully generated a random point in G1." << std::endl;
    ep_free(p);

    core_clean();
    std::cout << "=== Test Passed ===" << std::endl;
    return 0;
}
