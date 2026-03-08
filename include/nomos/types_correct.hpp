#pragma once

#include <string>
#include <vector>

extern "C" {
#include <relic/relic.h>
}

namespace nomos {

// Operation type
enum OP {
    OP_ADD = 0,
    OP_DEL = 1
};

// Update metadata sent from Gatekeeper to Server
struct UpdateMetadata {
    ep_t addr;                          // TSet address (elliptic curve point)
    std::vector<uint8_t> val;           // Encrypted (id||op)
    bn_t alpha;                         // Alpha value for cross-filtering
    std::vector<std::string> xtags;     // ℓ xtags (serialized elliptic curve points)

    UpdateMetadata() {
        ep_null(addr);
        bn_null(alpha);
    }

    ~UpdateMetadata() {
        ep_free(addr);
        bn_free(alpha);
    }
};

// OPRF Blinded Request (Client → Gatekeeper)
// Paper: Algorithm 3, Client side (lines 1-6)
struct BlindedRequest {
    std::vector<std::string> a;     // a_j = H(w_j)^{r_j}, j=1..n (serialized ep_t)
    std::vector<std::string> b;     // b_j = H(w_1||j||0)^{s_j}, j=1..m (serialized ep_t)
    std::vector<std::string> c;     // c_j = H(w_1||j||1)^{s_j}, j=1..m (serialized ep_t)
    std::vector<int> av;            // Access vector (I(w_1), ..., I(w_n))
};

// OPRF Blinded Response (Gatekeeper → Client)
// Paper: Algorithm 3, Gatekeeper side (lines 7-14)
struct BlindedResponse {
    std::string strap_prime;                        // strap' = a_1^{K_S} (serialized ep_t)
    std::vector<std::string> bstag_prime;           // bstag'_j = b_j^{K_T^{I_1} · gamma_j} (serialized)
    std::vector<std::string> delta_prime;           // delta'_j = c_j^{K_T^{I_1}} (serialized)
    std::vector<std::vector<std::string>> bxtrap_prime;  // bxtrap'_j[t] (serialized), j=2..n, t=1..k
    std::vector<uint8_t> env;                       // AE.Enc_{K_M}(rho_1..n, gamma_1..m)
};

// Token structure for search (after unblinding)
// Paper: Algorithm 3, Client side (lines 15-19)
struct SearchToken {
    ep_t strap;                                     // H(w1)^{K_S} (unblinded)
    std::vector<std::string> bstag;                 // bstag_j (unblinded, serialized)
    std::vector<std::string> delta;                 // delta_j (unblinded, serialized)
    std::vector<std::vector<std::string>> bxtrap;   // bxtrap_j[t] (unblinded, serialized)
    std::vector<uint8_t> env;                       // Encrypted (rho, gamma)

    SearchToken() {
        ep_null(strap);
    }

    ~SearchToken() {
        ep_free(strap);
    }
};

// Search result entry
struct SearchResultEntry {
    int j;                              // Index
    std::vector<uint8_t> sval;          // Encrypted value
    int cnt;                            // Match count
};

// TSet entry
struct TSetEntry {
    std::vector<uint8_t> val;           // Encrypted (id||op)
    bn_t alpha;                         // Alpha value

    TSetEntry() {
        bn_null(alpha);
    }

    ~TSetEntry() {
        bn_free(alpha);
    }
};

}  // namespace nomos
