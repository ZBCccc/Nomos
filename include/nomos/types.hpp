#pragma once

#include <string>
#include <vector>

namespace nomos {

struct Metadata {
    std::string addr;
    std::string val;
    std::string alpha;
    std::vector<std::string> xtag_list;
};

struct sEOp {
    int j;
    std::string sval;
    int cnt;
};

struct TrapdoorMetadata {
    std::vector<std::string> stokenList;
    std::vector<std::vector<std::string>> xtokenList;
};

}  // namespace nomos