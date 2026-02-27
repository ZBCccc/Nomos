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

// Token structure for search
struct SearchToken {
    ep_t strap;                                     // H(w1)^Ks
    std::vector<std::string> bstag;                 // Blinded stags (serialized)
    std::vector<std::string> delta;                 // Decryption masks (serialized)
    std::vector<std::vector<std::string>> bxtrap;   // Blinded xtraps [j][t] (serialized)
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
