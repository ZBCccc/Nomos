#pragma once

#include <string>
#include <vector>

namespace nomos {

struct Metadata {
    std::string addr;
    std::string val;
    std::string alpha;
    std::vector<std::string> xtag_list;
    // For testing: store plaintext id and op
    std::string id_plaintext;
    int op_plaintext;
    std::string keyword;  // The keyword this update is for
};

struct sEOp {
    int j;
    std::string sval;
    int cnt;
    std::string id;  // Decrypted id
};

struct TrapdoorMetadata {
    std::vector<std::string> stokenList;
    std::vector<std::vector<std::string>> xtokenList;
    std::string primary_keyword;  // For debugging
};

}  // namespace nomos